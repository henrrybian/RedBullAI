/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Cortex-M3 异常处理、SysTick 时基、USART1 与路径 A TIM2 更新中断入口（AI 生成）
  * @note    阶段 4 起 TIM2 更新中断 = 蜂鸣器软件 PWM 波形发生器（10 kHz）：
  *          中断内只做计数 + 比较 + PB2 置位/清零，禁止 printf、禁止除法与浮点、不阻塞。
  ******************************************************************************
  */
#include "main.h"
#include "stm32f1xx_it.h"
#include "buzzer.h"

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
  for (;;)
  {
  }
}

void MemManage_Handler(void)
{
  for (;;)
  {
  }
}

void BusFault_Handler(void)
{
  for (;;)
  {
  }
}

void UsageFault_Handler(void)
{
  for (;;)
  {
  }
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

/** HAL 1 ms 时基；HAL_RCC_ClockConfig 已按 72 MHz 重配 SysTick。 */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
}

void TIM2_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim2);
}

/** 路径 A（阶段 4）：TIM2 更新中断 = 波形发生器（10 kHz）。
 *  中断内只做「计数 + 比较 + PB2 置位/清零」；
 *  禁止 printf、禁止除法与浮点、禁止耗时循环（阶段 4 硬约束）；
 *  g_tim2_update_count 保留为 ISR 存活指示（GDB 可观察）。 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    g_tim2_update_count++;
    Buzzer_IsrUpdateOutput();
  }
}
