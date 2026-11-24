/**
  ******************************************************************************
  * @file    stm32f1xx_it.h
  * @brief   阶段 2 中断服务函数声明（AI 生成）
  * @note    向量名来自 ST 官方 startup_stm32f103xe.s（.weak TIM2_IRQHandler / USART1_IRQHandler）。
  ******************************************************************************
  */
#ifndef __STM32F1xx_IT_H
#define __STM32F1xx_IT_H

#ifdef __cplusplus
extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void USART1_IRQHandler(void);
void TIM2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_IT_H */
