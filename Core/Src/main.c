/**
  ******************************************************************************
  * @file    main.c
  * @brief   阶段 2 初始化层 + 阶段 3 启动日志 + 阶段 4 蜂鸣器业务：
  *          HAL_Init / SystemClock_Config / GPIO(含 PB2) / USART1 /
  *          路径 A TIM2 软件 PWM 时基(10 kHz) / 启动日志 / 启动鸣叫序列 /
  *          主循环调用 Buzzer_Tick()（AI 生成，用户零手写代码）
  * @source  结构按 ST 官方 STM32CubeF1 HAL 惯例组织；调用的 API 均已在
  *          Drivers/STM32F1xx_HAL_Driver/Inc 中逐项核对（见 @docs/stage2_hal_source.md）。
  * @note    硬件真相源：@docs/hardware_pins.md（锁定版）。
  *          {MCU}=STM32F103ZET6，HSE=8 MHz，目标 SYSCLK=72 MHz；
  *          USART1=PA9/PA10，115200 8N1；{PWM_PATH}=A，PB2 无 TIM 通道。
  *          阶段 3 只在时钟快照之后输出一行启动日志，不翻转 PB2、不实现鸣叫序列或变频逻辑。
  *          禁止把 PA13(SWDIO)/PA14(SWCLK) 复用为 GPIO；禁止任何 SWJ remap / DBGMCU 关闭配置。
  ******************************************************************************
  */
#include "main.h"
#include "stm32f1xx_it.h"
#include "debug_log.h"
#include "buzzer.h"
#include "feature_config.h"
#include "feature_switch.h"
#include "test_interface.h"

UART_HandleTypeDef huart1 = {0};
TIM_HandleTypeDef  htim2  = {0};

volatile uint32_t g_sysclk_hz = 0U;
volatile uint32_t g_hclk_hz   = 0U;
volatile uint32_t g_pclk1_hz  = 0U;
volatile uint32_t g_pclk2_hz  = 0U;
volatile uint32_t g_tim2_update_count = 0U;

/**
  * @brief  HSE 8 MHz → PLL x9 → SYSCLK 72 MHz；AHB /1，APB1 /2，APB2 /1；FLASH 2 wait states。
  *         HAL_RCC_ClockConfig 会更新 SystemCoreClock 并重配 SysTick（官方 hal_rcc.c 941/945 行）。
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef  clk = {0};

  osc.OscillatorType   = RCC_OSCILLATORTYPE_HSE;
  osc.HSEState         = RCC_HSE_ON;
  osc.HSEPredivValue   = RCC_HSE_PREDIV_DIV1;
  osc.PLL.PLLState     = RCC_PLL_ON;
  osc.PLL.PLLSource    = RCC_PLLSOURCE_HSE;
  osc.PLL.PLLMUL       = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK)
  {
    Error_Handler();
  }

  clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                       RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
  clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  clk.AHBCLKDivider  = RCC_HCLK_DIV1;   /* HCLK  = 72 MHz */
  clk.APB1CLKDivider = RCC_HCLK_DIV2;   /* PCLK1 = 36 MHz */
  clk.APB2CLKDivider = RCC_HCLK_DIV1;   /* PCLK2 = 72 MHz */
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  配置 USART1 需要的 PA9/PA10、阶段 4 蜂鸣器 PB2 与必要端口时钟。
  *         PA13/PA14 保持复位默认的 SWD 功能；不调用 __HAL_AFIO_REMAP_SWJ_*。
  */
void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();

  gpio.Pin   = GPIO_PIN_9;            /* USART1_TX */
  gpio.Mode  = GPIO_MODE_AF_PP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &gpio);

  gpio.Pin  = GPIO_PIN_10;             /* USART1_RX */
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &gpio);

  /* 阶段 4：PB2（BEEP/BOOT1，pin48）配置为推挽输出。复位默认 ODR2=0 → 上电即静音；
     鸣叫电平由 TIM2 中断的 Buzzer_IsrUpdateOutput() 驱动（高电平有效）。
     JP9 经 R25 10K 弱驱动，推挽输出可覆盖，无电平冲突（@docs/hardware_pins.md 70 行）。
     FEATURE_BUZZER_ENABLED=0 时不配置 PB2，引脚保持复位默认输入态，不产生任何电平。 */
#if FEATURE_BUZZER_ENABLED
  gpio.Pin   = GPIO_PIN_2;
  gpio.Mode  = GPIO_MODE_OUTPUT_PP;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &gpio);
#endif
}

void MX_USART1_UART_Init(void)
{
  huart1.Instance                    = USART1;
  huart1.Init.BaudRate               = DEBUG_USART_BAUDRATE;
  huart1.Init.WordLength             = UART_WORDLENGTH_8B;
  huart1.Init.StopBits               = UART_STOPBITS_1;
  huart1.Init.Parity                  = UART_PARITY_NONE;
  huart1.Init.Mode                   = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl               = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling            = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  路径 A 软件 PWM 时基（阶段 4 更新）：TIM2 更新中断 10 kHz。
  *         推导：72 MHz / (PSC+1) / (ARR+1) = 72 MHz / 72 / 100 = 10 kHz（设计上限）；
  *         软件 PWM 周期 = 10000/段频率 ticks，段频率 1–50 Hz → 分辨率 ≥10 级，
  *         完整推导与中断负载估算见 @docs/clock_tree.md 第 5 节。
  */
void MX_TIM2_Init(void)
{
  htim2.Instance               = TIM2;
  htim2.Init.Prescaler         = 72U - 1U;    /* PSC=71  → 1 MHz 计数频率 */
  htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim2.Init.Period            = 100U - 1U;   /* ARR=99  → 10 kHz 更新中断 */
  htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.RepetitionCounter = 0U;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
}

/** UART MSP：GPIO 已在 MX_GPIO_Init 配置，这里只开 USART1 时钟。 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1)
  {
    __HAL_RCC_USART1_CLK_ENABLE();
  }
}

/** TIM MSP：开 TIM2 时钟并使能更新中断向量。 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    __HAL_RCC_TIM2_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM2_IRQn, 1U, 0U);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);
  }
}

void Error_Handler(void)
{
  __disable_irq();
  for (;;)
  {
  }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
#if FEATURE_BUZZER_ENABLED
  MX_TIM2_Init();

  if (HAL_TIM_Base_Start_IT(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
#endif

  /* 运行期功能开关：必须在蜂鸣器初始化之后、任何开关操作之前；
     编译期关闭蜂鸣器时 Feature_Init() 仍要把所有槽位置为安全默认（全关）。 */
  Feature_Init();
#if FEATURE_TEST_ENABLED
  Test_Init();
#endif

  /* 运行态频率快照：SystemCoreClock 由官方 HAL_RCC_ClockConfig 更新，
     这里同时保留四个 HAL_RCC_Get*Freq() 实测值供日志与调试器观察。 */
  g_sysclk_hz = HAL_RCC_GetSysClockFreq();
  g_hclk_hz   = HAL_RCC_GetHCLKFreq();
  g_pclk1_hz  = HAL_RCC_GetPCLK1Freq();
  g_pclk2_hz  = HAL_RCC_GetPCLK2Freq();

  /* 阶段 3：USART1 已在上方完成初始化；关闭 stdout/stderr 缓冲后输出唯一一行启动日志。
     SYSCLK 取运行态快照 g_sysclk_hz（HAL_RCC_GetSysClockFreq()），
     对应工作流附录 B #13 的运行态 SYSCLK 验证；主循环内不再打印，避免高频刷屏。 */
  debug_uart_init();
  LOG_INFO("STAGE3 FW=%s BUILD=%s MCU=%s SYSCLK=%luHz UART=USART1/%lu-8N1 "
           "BEEP=PB2(pin48)/active-high PWM_PATH=A",
           FIRMWARE_VERSION, __DATE__ " " __TIME__, FIRMWARE_MCU_NAME,
           (unsigned long)g_sysclk_hz, (unsigned long)DEBUG_USART_BAUDRATE);

  /* 阶段 4：启动默认鸣叫序列（三短鸣 + 间歇 + 一长鸣 + 收尾静音，循环）。
     序列与参数见 @docs/buzzer_sequence.md；启动日志在其后追加一条 INFO。
     编译期 FEATURE_BUZZER_ENABLED=0 时整段不编译，PB2 常低、无 TIM2 中断。 */
#if FEATURE_BUZZER_ENABLED
  if (Feature_IsOn(FEATURE_ID_BUZZER) != 0U)
  {
    Buzzer_Start();
  }
#endif

#if FEATURE_TEST_ENABLED
  LOG_INFO("TEST interface ready: call Test_Request() or set g_test_request=1 in debugger");
#endif

  for (;;)
  {
#if FEATURE_BUZZER_ENABLED
    /* 先应用开关（状态未变则 O(1) 返回），再推进序列（非阻塞 O(1)）。 */
    Buzzer_ApplySwitch();
    Buzzer_Tick();
#endif
#if FEATURE_TEST_ENABLED
    /* 测试请求：未请求时只读一个 volatile 字节即返回；请求时跑一轮内置用例。 */
    Test_Poll();
#endif
  }
}
