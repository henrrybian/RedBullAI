/**
  ******************************************************************************
  * @file    stm32f1xx_hal_conf.h
  * @brief   阶段 2 HAL 模块配置（AI 生成）
  * @source  由 ST 官方 STM32CubeF1 V1.8.7 的
  *          Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_conf_template.h 裁剪而来。
  *          差异：仅保留阶段 2 所需模块开关与对应头文件包含；HSE_VALUE 保持官方 8 MHz。
  *          来源、版本、commit 与逐文件哈希见 @docs/stage2_hal_source.md。
  ******************************************************************************
  */
#ifndef __STM32F1xx_HAL_CONF_H
#define __STM32F1xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_EXTI_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* 未启用：ADC/CAN/CEC/CRC/DAC/ETH/HCD/I2C/I2S/IRDA/IWDG/MMC/NAND/NOR/
   PCCARD/PCD/RTC/SD/SMARTCARD/SPI/SRAM/USART/WWDG —— 阶段 2 无对应外设。 */

/* ########################## Oscillator Values adaptation ####################*/
#if !defined(HSE_VALUE)
#define HSE_VALUE    8000000U  /* @{HSE_MHZ}=8，红牛板 Y2 8MHz */
#endif
#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT    100U
#endif
#if !defined(HSI_VALUE)
#define HSI_VALUE    8000000U
#endif
#if !defined(LSI_VALUE)
#define LSI_VALUE    40000U
#endif
#if !defined(LSE_VALUE)
#define LSE_VALUE    32768U    /* 红牛板 Y1 32.768kHz，本阶段未启用 LSE */
#endif
#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT    5000U
#endif

/* ########################### System Configuration ######################### */
#define VDD_VALUE                    3300U
#define TICK_INT_PRIORITY              0x0FU
#define USE_RTOS                         0U
#define PREFETCH_ENABLE                  1U

#define USE_HAL_TIM_REGISTER_CALLBACKS    0U
#define USE_HAL_UART_REGISTER_CALLBACKS   0U
#define USE_HAL_RCC_REGISTER_CALLBACKS   0U
#define USE_HAL_GPIO_REGISTER_CALLBACKS  0U

/* USE_FULL_ASSERT 未启用；如需开启，须同时实现 assert_failed()。 */
/* #define USE_FULL_ASSERT    1U */

/* Includes ------------------------------------------------------------------*/
#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32f1xx_hal_rcc.h"
#endif
#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32f1xx_hal_gpio.h"
#endif
#ifdef HAL_EXTI_MODULE_ENABLED
#include "stm32f1xx_hal_exti.h"
#endif
#ifdef HAL_DMA_MODULE_ENABLED
#include "stm32f1xx_hal_dma.h"
#endif
#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32f1xx_hal_cortex.h"
#endif
#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32f1xx_hal_flash.h"
#endif
#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32f1xx_hal_pwr.h"
#endif
#ifdef HAL_TIM_MODULE_ENABLED
#include "stm32f1xx_hal_tim.h"
#endif
#ifdef HAL_UART_MODULE_ENABLED
#include "stm32f1xx_hal_uart.h"
#endif

/* Exported macro ------------------------------------------------------------*/
#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_HAL_CONF_H */
