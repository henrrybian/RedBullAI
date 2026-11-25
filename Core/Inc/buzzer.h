/**
  ******************************************************************************
  * @file    buzzer.h
  * @brief   阶段 4：路径 A 软件 PWM 蜂鸣器业务接口（AI 生成，用户零手写代码）
  * @source  调用的 HAL 类型均来自 ST 官方 STM32CubeF1 V1.8.7 头文件
  *          （stm32f1xx_hal.h / stm32f1xx_hal_gpio.h，见 @docs/stage2_hal_source.md）。
  * @note    硬件真相源：@docs/hardware_pins.md（锁定版，不得改写）。
  *          BEEP/BOOT1 = PB2（GPIOB / GPIO_PIN_2，LQFP144 pin 48）；
  *          有源蜂鸣器（11-12 GDB 实测）、高电平有效（ODR 0x14→0x10 实测）；
  *          PB2 无任何 TIMx_CHy 复用 → {PWM_PATH}=A（TIM 更新中断 + GPIO 软件 PWM）。
  *
  *          职责二分（本阶段架构约定）：
  *            - TIM2 更新中断（10 kHz）= 波形发生器：Buzzer_IsrUpdateOutput()
  *              中断内只做「读参数 + 比较 + PB2 置位/清零」；
  *              禁止 printf、禁止除法与浮点、禁止耗时循环。
  *            - 主循环 Buzzer_Tick() = 序列推进器：基于 HAL_GetTick() 的非阻塞
  *              O(1) 状态机；段到期才切换参数并输出一条 INFO 日志。
  *
  *          有源蜂鸣器语义硬约束：PWM 只表达「通断节奏 + 占空比」；
  *          「目标频率」对有源蜂鸣器的实际听感是断续节奏，不代表音调；
  *          本模块严禁生成任何「变频变调」逻辑或承诺（工作流 273/495 行）。
  *          有效电平硬约束：静音 = PB2 输出低；鸣叫段按占空比输出高/低，不得反相。
  ******************************************************************************
  */
#ifndef __BUZZER_H
#define __BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/** 蜂鸣器驱动引脚（锁定版：PB2，pin 48，高电平有效，5V 供电经 JP5 使能）。 */
#define BUZZER_GPIO_PORT   GPIOB
#define BUZZER_GPIO_PIN    GPIO_PIN_2

/* ---- 单步观察变量（volatile，Debug 构建 -O0 -g3 下不被优化，可 GDB/变量视图直读） ---- */
extern volatile uint32_t g_buzzer_seg_index;   /* 当前段索引（0 起；序列表段号 = 索引 + 1） */
extern volatile uint32_t g_buzzer_freq_hz;     /* 当前段目标频率 Hz（有源=断续节奏，非音调） */
extern volatile uint32_t g_buzzer_duty_pct;    /* 当前段实际占空比（%，高电平时间占比） */
extern volatile uint32_t g_buzzer_remain_ms;   /* 当前段剩余时长（ms） */

/**
  * @brief  启动默认鸣叫序列：装载段 1 并以当前时刻为时基；仅允许主线程调用一次。
  * @note   必须在 MX_TIM2_Init() + HAL_TIM_Base_Start_IT() 之后、调试日志初始化之后调用。
  */
void Buzzer_Start(void);

/**
  * @brief  运行期开关：经 @feature_switch.h 设置 FEATURE_ID_BUZZER 并立即生效。
  *         on=0 → 关闭：PB2 强制常低（静音）、序列停止推进、日志仅一次；
  *         非 0 → 打开：若序列未启动则从段 1 启动，否则从当前段恢复。
  * @note   仅允许主线程调用；不要在中段服务函数内调用。
  */
void Buzzer_SetEnabled(uint8_t on);

/**
  * @brief  把开关表的当前状态应用到蜂鸣器外设（主循环每轮调用一次，O(1)）。
  *         用于捕获调试器直接写 g_feature_state[] 造成的外部切换；
  *         状态未变化时立即返回，不产生日志与多余 GPIO 写。
  */
void Buzzer_ApplySwitch(void);

/**
  * @brief  序列推进器（主循环每个循环调用一次）：O(1)、无阻塞、无动态内存。
  * @note   段未到期时只刷新剩余时长；到期时切换段参数并输出一条 INFO 日志。
  *         严禁在中断内调用；严禁改为轮询等待（禁止 HAL_Delay/忙等）。
  */
void Buzzer_Tick(void);

/**
  * @brief  波形发生器（仅供 TIM2 更新中断调用）：按当前段目标频率/占空比的
  *         预比较值决定 PB2 输出高或低。中断内无除法、无浮点、无 printf。
  */
void Buzzer_IsrUpdateOutput(void);

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_H */
