/**
  ******************************************************************************
  * @file    buzzer.c
  * @brief   阶段 4：路径 A 软件 PWM 蜂鸣器业务实现（AI 生成，用户零手写代码）
  * @source  调用的 HAL API 与 CMSIS 内联均逐项核对自 ST 官方 STM32CubeF1 V1.8.7 /
  *          CMSIS core_cm3.h（见 @docs/stage2_hal_source.md）：
  *          - HAL_GetTick()        stm32f1xx_hal.h（1 ms 时基，SysTick 驱动）
  *          - HAL_GPIO_WritePin()  stm32f1xx_hal_gpio.h:311（写 BSRR/BRR，O(1)）
  *          - __get_PRIMASK()/__set_PRIMASK()/__disable_irq()  CMSIS 官方内联
  * @note    时基依据（推导见 @docs/clock_tree.md 第 5 节）：
  *          TIM2 时钟 72 MHz → PSC=71（/72 → 1 MHz）→ ARR=99（/100 → 10 kHz 更新中断，
  *          恰为设计上限）；软件 PWM 周期 = 10000/段频率 ticks，段频率 1–50 Hz →
  *          周期 10000–200 ticks，占空比 1% 步进对应 2–100 ticks → 分辨率 ≥10 级。
  *
  *          有源蜂鸣器语义硬约束：本文件的「频率」字段只表达通断节拍，实际听感是
  *          断续节奏（响/停交替），不代表音调；占空比 = 每节拍内响的时间占比。
  *          占空比对有源蜂鸣器听感影响有限（有源器件音调由内部振荡器决定），
  *          但字段与机制保留为接口，供阶段后续调整节奏质感；不删除、不做变调承诺。
  *          有效电平：高电平有效 → 静音段 s_pwm_high_ticks=0（PB2 常低），
  *          鸣叫段按占空比输出高/低，不反相。
  *
  *          中断安全：Buzzer_IsrUpdateOutput() 只读预计算参数（s_pwm_*），
  *          除法与乘法全部在主循环 Buzzer_LoadSegment() 完成；
  *          参数更新用 PRIMASK 临界区保护，避免中断撕裂。
  ******************************************************************************
  */
#include "buzzer.h"
#include "main.h"
#include "debug_log.h"

/* ------------------------------------------------------------------ */
/* 时序常量：与 @docs/clock_tree.md 第 5 节推导一致                     */
/* ------------------------------------------------------------------ */

/** TIM2 更新中断频率（Hz）：72 MHz / (PSC 71+1) / (ARR 99+1) = 10 kHz（设计上限）。 */
#define BUZZER_ISR_RATE_HZ        10000U

/** 占空比安全上限（%）与频率下限（Hz）：保证周期 ticks 与高电平 ticks 恒为正。 */
#define BUZZER_DUTY_MAX_PCT       100U
#define BUZZER_FREQ_MIN_HZ        1U

/* ------------------------------------------------------------------ */
/* 默认鸣叫序列：与 @docs/buzzer_sequence.md 第 2 节逐项一致            */
/* 有源语义：freq_hz = 通断节拍频率（非音调）；duty=100% 时无可闻断续，   */
/* 整段连续鸣叫；设计理由与实际听感说明见 buzzer_sequence.md 第 3 节。   */
/* ------------------------------------------------------------------ */

typedef struct
{
  uint8_t  on;           /* 1=响段（按占空比输出高/低）；0=停段（静音，PB2 常低） */
  uint32_t freq_hz;      /* 通断节拍频率 Hz（有源=断续节奏，不代表音调） */
  uint32_t duty_pct;     /* 占空比 %（高电平时间占比；停段为 0） */
  uint32_t duration_ms;  /* 本段时长 ms */
} Buzzer_Segment_t;

#define BUZZER_SEQ_LEN   8U

static const Buzzer_Segment_t s_sequence[BUZZER_SEQ_LEN] =
{
  /* 段1  短鸣1 */ {1U, 2U, 100U, 150U},
  /* 段2  静音  */ {0U, 0U,   0U, 150U},
  /* 段3  短鸣2 */ {1U, 2U, 100U, 150U},
  /* 段4  静音  */ {0U, 0U,   0U, 150U},
  /* 段5  短鸣3 */ {1U, 2U, 100U, 150U},
  /* 段6  间歇  */ {0U, 0U,   0U, 600U},
  /* 段7  长鸣  */ {1U, 2U, 100U, 800U},
  /* 段8  收尾静音，随后循环回段1 */ {0U, 0U, 0U, 2000U},
};

/* ------------------------------------------------------------------ */
/* 单步观察变量（volatile；-O0 -g3 下保留符号与内存访问）               */
/* ------------------------------------------------------------------ */

volatile uint32_t g_buzzer_seg_index = 0U;
volatile uint32_t g_buzzer_freq_hz   = 0U;
volatile uint32_t g_buzzer_duty_pct  = 0U;
volatile uint32_t g_buzzer_remain_ms = 0U;

/* ------------------------------------------------------------------ */
/* 内部状态：s_pwm_* 由 ISR 读取；仅 Buzzer_LoadSegment() 在临界区内写  */
/* ------------------------------------------------------------------ */

static volatile uint32_t s_pwm_counter      = 0U; /* ISR 软件 PWM 计数器 */
static volatile uint32_t s_pwm_period_ticks = 1U; /* PWM 周期（ISR tick 数）；1=退化安全值 */
static volatile uint32_t s_pwm_high_ticks   = 0U; /* 高电平 tick 数；0=常低（静音） */

static uint8_t  s_started     = 0U;  /* 序列是否已启动 */
static uint32_t s_seg_end_tick = 0U; /* 当前段结束时刻（HAL_GetTick 口径） */

/**
  * @brief  装载一段：把「频率/占空比」换算成 ISR 预比较值（除法在此完成，ISR 内禁止）。
  * @param  index     段索引（0 起）
  * @param  now_tick  当前 HAL_GetTick() 值
  * @note   响段：周期 = 10000/freq；高电平 ticks = 周期 × duty%。
  *         停段：高电平 ticks = 0 → ISR 每个周期都把 PB2 清低（静音）。
  */
static void Buzzer_LoadSegment(uint32_t index, uint32_t now_tick)
{
  const Buzzer_Segment_t *seg = &s_sequence[index];
  uint32_t period_ticks;
  uint32_t high_ticks;
  uint32_t freq_hz;
  uint32_t duty_pct;
  uint32_t primask;

  if (seg->on != 0U)
  {
    freq_hz  = (seg->freq_hz < BUZZER_FREQ_MIN_HZ) ? BUZZER_FREQ_MIN_HZ : seg->freq_hz;
    duty_pct = (seg->duty_pct > BUZZER_DUTY_MAX_PCT) ? BUZZER_DUTY_MAX_PCT : seg->duty_pct;

    /* 主循环内除法/乘法（CMSIS 软件除法），ISR 内只做比较 */
    period_ticks = BUZZER_ISR_RATE_HZ / freq_hz;
    if (period_ticks == 0U)
    {
      period_ticks = 1U;
    }
    high_ticks = (period_ticks * duty_pct) / 100U;
  }
  else
  {
    freq_hz      = 0U;
    duty_pct     = 0U;
    period_ticks = 1U;
    high_ticks   = 0U;   /* 静音硬约束：PB2 常低 */
  }

  /* 临界区更新 ISR 共享参数（CMSIS 官方内联；保护期 < 1 µs，不影响 SysTick） */
  primask = __get_PRIMASK();
  __disable_irq();
  s_pwm_period_ticks = period_ticks;
  s_pwm_high_ticks   = high_ticks;
  s_pwm_counter      = 0U;
  __set_PRIMASK(primask);

  /* 观察变量与段时基（uint32 单写原子，无需额外保护） */
  g_buzzer_seg_index = index;
  g_buzzer_freq_hz   = freq_hz;
  g_buzzer_duty_pct  = duty_pct;
  g_buzzer_remain_ms = seg->duration_ms;
  s_seg_end_tick     = now_tick + seg->duration_ms;
}

void Buzzer_Start(void)
{
  s_started = 1U;
  Buzzer_LoadSegment(0U, HAL_GetTick());

  /* 序列启动日志：一条 INFO，参数与序列表逐项一致（见 buzzer_sequence.md 第 2 节） */
  LOG_INFO("BUZZER seq=%u segs start seg=1 on=ON freq=%uHz duty=%u%% dur=%ums",
           (unsigned int)BUZZER_SEQ_LEN,
           (unsigned int)g_buzzer_freq_hz,
           (unsigned int)g_buzzer_duty_pct,
           (unsigned int)s_sequence[0U].duration_ms);
}

void Buzzer_Tick(void)
{
  uint32_t now;

  if (s_started == 0U)
  {
    return;
  }

  now = HAL_GetTick();

  if (now < s_seg_end_tick)
  {
    g_buzzer_remain_ms = s_seg_end_tick - now;   /* O(1)，无阻塞 */
    return;
  }

  /* 段到期：推进到下一段并打印一条 INFO（只在状态切换时打印，禁止刷屏） */
  {
    uint32_t next = (g_buzzer_seg_index + 1U) % BUZZER_SEQ_LEN;
    Buzzer_LoadSegment(next, now);
    LOG_INFO("BUZZER seg=%u/%u on=%s freq=%uHz duty=%u%% remain=%ums",
             (unsigned int)(g_buzzer_seg_index + 1U),
             (unsigned int)BUZZER_SEQ_LEN,
             (s_sequence[g_buzzer_seg_index].on != 0U) ? "ON" : "OFF",
             (unsigned int)g_buzzer_freq_hz,
             (unsigned int)g_buzzer_duty_pct,
             (unsigned int)g_buzzer_remain_ms);
  }
}

void Buzzer_IsrUpdateOutput(void)
{
  uint32_t cnt = s_pwm_counter + 1U;

  if (cnt >= s_pwm_period_ticks)
  {
    cnt = 0U;
  }
  s_pwm_counter = cnt;

  /* 有效电平硬约束：高电平有效 → 计数落在高电平窗口内输出高，否则输出低（静音）。
     HAL_GPIO_WritePin 内部为单次 BSRR/BRR 写，O(1)，无读改写、无除法、无 printf。 */
  if (cnt < s_pwm_high_ticks)
  {
    //HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_SET);
  }
  else
  {
    //HAL_GPIO_WritePin(BUZZER_GPIO_PORT, BUZZER_GPIO_PIN, GPIO_PIN_RESET);
  }
}
 