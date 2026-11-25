/**
  ******************************************************************************
  * @file    main.h
  * @brief   阶段 2 初始化层全局声明（AI 生成）
  * @note    硬件真相源：@docs/hardware_pins.md（锁定版），本文件不重复定义锁定引脚值。
  *          禁止把 PA13/SWDIO、PA14/SWCLK 复用为 GPIO；禁止 SWJ remap / DBGMCU 关闭配置。
  ******************************************************************************
  */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

#define DEBUG_USART_BAUDRATE   115200U   /* @{DEBUG_BAUD}=115200 8N1 */

extern UART_HandleTypeDef huart1;        /* USART1，PA9(TX)/PA10(RX) */
extern TIM_HandleTypeDef  htim2;         /* 路径 A 基础时基，TIM2 更新中断 */

extern volatile uint32_t g_sysclk_hz;
extern volatile uint32_t g_hclk_hz;
extern volatile uint32_t g_pclk1_hz;
extern volatile uint32_t g_pclk2_hz;
extern volatile uint32_t g_tim2_update_count;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init(void);
void MX_TIM2_Init(void);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
