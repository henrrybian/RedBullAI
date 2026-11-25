/**
  ******************************************************************************
  * @file    test_interface.h
  * @brief   可注册测试接口（AI 生成，用户零手写代码）
  * @note    用途：为零手写代码流程提供「加测试 = 写一个返回 int 的函数 + 注册一行」的入口。
  *          用例函数约定：返回 0 表示 PASS，非 0 表示 FAIL 码（1..N 自定义）。
  *          运行结果通过 USART1 的 LOG_INFO/LOG_ERROR 输出，并写入 volatile 计数器供 GDB 读取。
  *          两种方式触发：
  *            1) 代码中调用 Test_RunAll()；
  *            2) 断点在 main 循环，调试器置 g_test_request=1 后继续运行（Test_Poll 自动执行）。
  *          约束：用例在主循环上下文执行，禁止在用例内调用阻塞延时或关闭中断；
  *          禁止在中断服务函数内调用本接口。
  ******************************************************************************
  */
#ifndef __TEST_INTERFACE_H
#define __TEST_INTERFACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 测试用例函数类型：0=PASS；非 0=FAIL 码。 */
typedef int (*TestFn_t)(void);

/** 运行统计（volatile，GDB/变量视图可直读） */
extern volatile uint32_t g_test_pass_count;
extern volatile uint32_t g_test_fail_count;
extern volatile uint32_t g_test_last_fail;   /* 最近一次 Test_RunAll() 的失败数 */
extern volatile uint32_t g_test_last_rc;     /* 最近一个用例的返回码 */
extern volatile uint8_t  g_test_request;     /* 置 1 请求执行一次全量测试 */

/** 初始化测试框架并注册内置用例；上电调用一次。 */
void Test_Init(void);

/** 注册用例：返回 0=成功，-1=注册表满。 */
int Test_Register(const char *name, TestFn_t fn);

/** 运行全部已注册用例；返回失败数（0=全通过）。 */
uint32_t Test_RunAll(void);

/** 请求一次测试（置 g_test_request=1），由 Test_Poll() 在下一轮主循环执行。 */
void Test_Request(void);

/** 主循环轮询：仅当收到请求时执行一次 Test_RunAll()，否则 O(1) 返回。 */
void Test_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* __TEST_INTERFACE_H */
