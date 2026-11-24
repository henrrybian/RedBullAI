/**
  ******************************************************************************
  * @file    debug_uart.c
  * @brief   阶段 3：printf -> USART1(huart1) 阻塞式重定向（AI 生成，用户零手写代码）
  * @source  HAL_UART_Transmit 原型逐字核对自 ST 官方 STM32CubeF1 V1.8.7
  *          Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_uart.h:749：
  *          HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart,
  *                                             const uint8_t *pData, uint16_t Size,
  *                                             uint32_t Timeout);
  *          HAL_OK / HAL_ERROR 见 stm32f1xx_hal_def.h:40-41。
  *
  * @note    链接规格实测（阶段 3 步骤 2）：
  *          cmake/gcc-arm-none-eabi.cmake:20 = -specs=nano.specs -specs=nosys.specs。
  *          - newlib-nano：libg_nano.a 的 libc_a-writer.o 为 U _write / T _write_r；
  *            sys/unistd.h:277 声明 _READ_WRITE_RETURN_TYPE _write(int, const void *, size_t)，
  *            sys/config.h:83 把它定义为 _ssize_t。
  *          - libnosys.a：write.o 单独提供 T _write 桩（只失败返回），且 libnosys 中
  *            __io_putchar 符号命中数为 0 —— 在 nosys.specs 下 libnosys 的 _write 不会
  *            回调 __io_putchar，单独实现 __io_putchar 无法让 printf 输出。
  *          故选择强定义 _write：链接器不再抽取 libnosys 的 write.o，
  *          既无 undefined reference 也无符号重复；nosys.specs 仍为必需
  *          （提供 _close/_lseek/_read/_exit/_sbrk 等桩），工具链文件无需改动。
  *
  *          硬约束：本文件只在主线程上下文被调用；任何中断服务函数禁止调用 printf / LOG_*。
  *          本阶段不包含蜂鸣器业务、不配置 PB2、不复用 PA13/PA14、不做任何 SWJ remap/DBGMCU 配置。
  ******************************************************************************
  */
#include "debug_log.h"
#include "main.h"
#include <sys/unistd.h>
#include <stddef.h>

/** 单次发送超时（ms）：115200 8N1 下 100 ms 可覆盖约 1000 字节，远超单行日志长度。 */
#define DEBUG_UART_TX_TIMEOUT_MS   100U

void debug_uart_init(void)
{
  /* 裸机下 _isatty() 由 nosys 返回 0，stdout 默认全缓冲；
     显式置为无缓冲，保证 printf 的每一行立即经 _write 发出。 */
  (void)setvbuf(stdout, NULL, _IONBF, 0U);
  (void)setvbuf(stderr, NULL, _IONBF, 0U);
}

/**
  * @brief  newlib 底层写调用：把 stdout/stderr 的字节流阻塞式送到 huart1（USART1）。
  * @param  fd    文件描述符（stdout=1 / stderr=2；本工程不区分，统一走 USART1）
  * @param  buf   待发送缓冲区
  * @param  nbyte 字节数
  * @return 成功返回已发送字节数；失败返回 -1（与 newlib 的 ssize_t 语义一致）
  */
_READ_WRITE_RETURN_TYPE _write(int fd, const void *buf, size_t nbyte)
{
  (void)fd;

  if ((buf == NULL) || (nbyte == 0U) || (nbyte > 0xFFFFU))
  {
    return 0;
  }

  if (HAL_UART_Transmit(&huart1, (const uint8_t *)buf,
                        (uint16_t)nbyte, DEBUG_UART_TX_TIMEOUT_MS) != HAL_OK)
  {
    return -1;
  }

  return (_READ_WRITE_RETURN_TYPE)nbyte;
}
