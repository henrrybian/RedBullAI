/**
  ******************************************************************************
  * @file    feature_switch.c
  * @brief   运行期功能开关实现（AI 生成，用户零手写代码）
  * @note    只维护状态表，不直接操作任何外设；外设在自己的模块里响应状态变化。
  ******************************************************************************
  */
#include "feature_switch.h"
#include "feature_config.h"

volatile uint8_t g_feature_state[FEATURE_ID_COUNT] = {0U};

void Feature_Init(void)
{
#if FEATURE_BUZZER_ENABLED
  g_feature_state[FEATURE_ID_BUZZER] = (FEATURE_BUZZER_DEFAULT_ON != 0U) ? 1U : 0U;
#else
  g_feature_state[FEATURE_ID_BUZZER] = 0U;
#endif
}

void Feature_Set(FeatureId_t id, uint8_t on)
{
  if ((uint32_t)id < (uint32_t)FEATURE_ID_COUNT)
  {
    g_feature_state[(uint32_t)id] = (on != 0U) ? 1U : 0U;
  }
}

void Feature_Toggle(FeatureId_t id)
{
  if ((uint32_t)id < (uint32_t)FEATURE_ID_COUNT)
  {
    g_feature_state[(uint32_t)id] ^= 1U;
  }
}

uint8_t Feature_IsOn(FeatureId_t id)
{
  if ((uint32_t)id >= (uint32_t)FEATURE_ID_COUNT)
  {
    return 0U;
  }
  return g_feature_state[(uint32_t)id];
}
