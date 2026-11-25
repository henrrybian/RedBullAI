/**
  ******************************************************************************
  * @file    feature_config.h
  * @brief   功能模块编译期总开关（集中配置；可由 CMake -D 覆盖，AI 生成，用户零手写代码）
  * @note    一模块一宏：置 1 = 该模块编译进固件并参与上电调度；
  *          置 0 = 该模块不启动、相关 GPIO 保持复位默认电平（蜂鸣器 = PB2 常低静音）。
  *          本文件只管「编不编进来 / 上电是否默认运行」；
  *          运行中的开 / 关由 @feature_switch.h 提供接口，两层互不混淆。
  *          新增模块按文件末尾模板追加即可，不影响既有阶段 0–5 的验收结论。
  ******************************************************************************
  */
#ifndef __FEATURE_CONFIG_H
#define __FEATURE_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* 阶段 4 蜂鸣器 PWM 业务：1=编译并上电调度；0=PB2 常低、TIM2 不启动更新中断 */
#ifndef FEATURE_BUZZER_ENABLED
#define FEATURE_BUZZER_ENABLED   1U
#endif

/* 蜂鸣器上电默认运行态（仅在 FEATURE_BUZZER_ENABLED=1 时有效） */
#ifndef FEATURE_BUZZER_DEFAULT_ON
#define FEATURE_BUZZER_DEFAULT_ON   1U
#endif

/* 测试接口框架：1=编译注册/运行接口；0=整个测试框架不参与链接 */
#ifndef FEATURE_TEST_ENABLED
#define FEATURE_TEST_ENABLED   1U
#endif

/* 后续模块追加模板（保持「一模块一宏」）：
   #ifndef FEATURE_XXX_ENABLED
   #define FEATURE_XXX_ENABLED   0U
   #endif
*/

#ifdef __cplusplus
}
#endif

#endif /* __FEATURE_CONFIG_H */
