/**
  ******************************************************************************
  * @file    debug_log.h
  * @brief   阶段 3：串口 Debug 日志等级宏与 printf 重定向初始化声明（AI 生成，用户零手写代码）
  * @note    硬件真相源：@docs/hardware_pins.md（锁定版，不得改写）。
  *          Debug 串口 USART1 = PA9(pin101, TX) / PA10(pin102, RX)，115200 8N1，无流控；
  *          huart1 在 Core/Src/main.c 中定义，并由 MX_USART1_UART_Init() 完成初始化。
  *
  *          日志约定：单行、低频、仅状态/启动信息，不含敏感信息；每条日志以 \r\n 结束。
  *          严禁在中断服务函数中调用 printf 或任何 LOG_* 宏；
  *          严禁在主循环、TIM 更新中断或任何周期性回调中打印（避免阶段 5 单步调试刷屏）。
  *
  *          本阶段不实现蜂鸣器鸣叫/变频逻辑（阶段 4），不创建任何调试前端配置（阶段 5）。
  ******************************************************************************
  */
#ifndef __DEBUG_LOG_H
#define __DEBUG_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

/** 固件版本（阶段 3） */
#define FIRMWARE_VERSION    "0.3.0"
/** MCU 型号，取自 @docs/hardware_pins.md 第 1 节 {MCU} */
#define FIRMWARE_MCU_NAME   "STM32F103ZET6"

/* 编译期日志等级：0=全关 1=ERROR 2=WARN 3=INFO 4=DEBUG */
#define DEBUG_LOG_LEVEL_OFF     0
#define DEBUG_LOG_LEVEL_ERROR   1
#define DEBUG_LOG_LEVEL_WARN    2
#define DEBUG_LOG_LEVEL_INFO    3
#define DEBUG_LOG_LEVEL_DEBUG   4

/*
 * CMakeLists.txt 通过 -DDEBUG_LOG_LEVEL=N 注入：
 *   Debug   默认 4 -> ERROR/WARN/INFO/DEBUG 全开（含 INFO/DEBUG）
 *   Release 默认 2 -> 仅 ERROR/WARN，INFO/DEBUG 被编译掉
 * 可随时用 cmake -DDEBUG_LOG_LEVEL=0 完全关闭。
 * 以下分支只作为脱离 CMake 单独编译时的兜底，不影响 CMake 注入值。
 */
#ifndef DEBUG_LOG_LEVEL
#  if defined(DEBUG) || defined(_DEBUG)
#    define DEBUG_LOG_LEVEL   DEBUG_LOG_LEVEL_DEBUG
#  else
#    define DEBUG_LOG_LEVEL   DEBUG_LOG_LEVEL_WARN
#  endif
#endif

/**
  * @brief  初始化 printf 重定向：关闭 stdout/stderr 缓冲，保证每条日志立即发送到 USART1，
  *         使单步调试时「终端出现下一行」与语句执行顺序严格一致。
  * @note   必须在 MX_USART1_UART_Init() 之后、首次 printf 之前调用；只允许主线程（非中断）调用。
  */
void debug_uart_init(void);

#if DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...)  do { (void)printf("[ERR] " fmt "\r\n", ##__VA_ARGS__); } while (0U)
#else
#define LOG_ERROR(fmt, ...)  ((void)0U)
#endif

#if DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...)   do { (void)printf("[WRN] " fmt "\r\n", ##__VA_ARGS__); } while (0U)
#else
#define LOG_WARN(fmt, ...)   ((void)0U)
#endif

#if DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...)   do { (void)printf("[INF] " fmt "\r\n", ##__VA_ARGS__); } while (0U)
#else
#define LOG_INFO(fmt, ...)   ((void)0U)
#endif

#if DEBUG_LOG_LEVEL >= DEBUG_LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...)  do { (void)printf("[DBG] " fmt "\r\n", ##__VA_ARGS__); } while (0U)
#else
#define LOG_DEBUG(fmt, ...)  ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_LOG_H */
