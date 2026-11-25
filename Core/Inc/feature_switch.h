/**
  ******************************************************************************
  * @file    feature_switch.h
  * @brief   运行期功能开关接口（AI 生成，用户零手写代码）
  * @note    每个功能模块占一个 FEATURE_ID_xxx 槽位，状态存在 volatile 数组中，
  *          Debug 构建 -O0 -g3 下可在 GDB / Cortex-Debug 变量视图直接读写：
  *            (gdb) p g_feature_state[FEATURE_ID_BUZZER]
  *            (gdb) set g_feature_state[FEATURE_ID_BUZZER] = 0
  *          代码中统一走 Feature_Set/Toggle/IsOn，保证边界检查与单一真相源。
  *          蜂鸣器关闭时由 buzzer.c 强制 PB2 常低（高电平有效 → 静音）。
  ******************************************************************************
  */
#ifndef __FEATURE_SWITCH_H
#define __FEATURE_SWITCH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 功能槽位：新增模块在枚举末尾追加一个 ID（FEATURE_ID_COUNT 之前）。 */
typedef enum
{
  FEATURE_ID_BUZZER = 0U,
  FEATURE_ID_COUNT
} FeatureId_t;

/** 运行态开关表：0=关，1=开；volatile 便于调试器观察与修改。 */
extern volatile uint8_t g_feature_state[FEATURE_ID_COUNT];

/** 按 feature_config.h 的编译期默认值初始化全部槽位；上电调用一次。 */
void Feature_Init(void);

/** 设置槽位状态：on=0 关，非 0 开；越界 ID 静默忽略。 */
void Feature_Set(FeatureId_t id, uint8_t on);

/** 翻转槽位状态（0<->1）；越界 ID 静默忽略。 */
void Feature_Toggle(FeatureId_t id);

/** 读取槽位状态；越界 ID 返回 0（关）。 */
uint8_t Feature_IsOn(FeatureId_t id);

#ifdef __cplusplus
}
#endif

#endif /* __FEATURE_SWITCH_H */
