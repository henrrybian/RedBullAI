/**
  ******************************************************************************
  * @file    test_interface.c
  * @brief   可注册测试接口实现（AI 生成，用户零手写代码）
  * @note    内置用例验证「功能开关」这一新增接口本身；后续模块按如下三步接入：
  *            1) 写一个 static int MyTest(void)，返回 0=PASS / 非 0=FAIL 码；
  *            2) 在模块初始化里调用 Test_Register("module_case", MyTest)；
  *            3) 在 main 断点置 g_test_request=1（或代码里调 Test_Request()）运行。
  *          用例只在主循环上下文运行，绝不入中断；日志低频、仅测试是非。
  ******************************************************************************
  */
#include "test_interface.h"
#include "feature_config.h"
#include "feature_switch.h"
#include "buzzer.h"
#include "debug_log.h"

#define TEST_MAX_CASES   8U

typedef struct
{
  const char *name;
  TestFn_t    fn;
} TestCase_t;

static TestCase_t s_cases[TEST_MAX_CASES];
static uint32_t   s_count = 0U;

volatile uint32_t g_test_pass_count = 0U;
volatile uint32_t g_test_fail_count = 0U;
volatile uint32_t g_test_last_fail  = 0U;
volatile uint32_t g_test_last_rc    = 0U;
volatile uint8_t  g_test_request    = 0U;

/* ------------------------------------------------------------------ */
/* 内置用例 1：开关表读写自检（不依赖具体外设，任何配置下都注册）       */
/* ------------------------------------------------------------------ */
static int Test_SwitchTableSanity(void)
{
  uint8_t saved = g_feature_state[FEATURE_ID_BUZZER];

  Feature_Set(FEATURE_ID_BUZZER, 0U);
  if (Feature_IsOn(FEATURE_ID_BUZZER) != 0U)
  {
    Feature_Set(FEATURE_ID_BUZZER, saved);
    return 1;
  }

  Feature_Toggle(FEATURE_ID_BUZZER);
  if (Feature_IsOn(FEATURE_ID_BUZZER) == 0U)
  {
    Feature_Set(FEATURE_ID_BUZZER, saved);
    return 2;
  }

  Feature_Set(FEATURE_ID_BUZZER, saved);
  return 0;
}

/* ------------------------------------------------------------------ */
/* 内置用例 2：关闭蜂鸣器必须立即把 PB2 拉低（高电平有效 → 静音）        */
/* 测试自身会保存/恢复原开关状态，不改变上电默认行为                    */
/* ------------------------------------------------------------------ */
static int Test_BuzzerOffForcesMute(void)
{
  uint8_t saved = Feature_IsOn(FEATURE_ID_BUZZER);

  Buzzer_SetEnabled(0U);
  if (Feature_IsOn(FEATURE_ID_BUZZER) != 0U)
  {
    Feature_Set(FEATURE_ID_BUZZER, saved);
    Buzzer_ApplySwitch();
    return 1;
  }
  if ((BUZZER_GPIO_PORT->ODR & BUZZER_GPIO_PIN) != 0U)
  {
    Feature_Set(FEATURE_ID_BUZZER, saved);
    Buzzer_ApplySwitch();
    return 2;
  }

  Buzzer_SetEnabled(saved);
  return 0;
}

/* ------------------------------------------------------------------ */
/* 内置用例 3：打开蜂鸣器后开关位为 1                                   */
/* ------------------------------------------------------------------ */
static int Test_BuzzerOnSetsFlag(void)
{
  uint8_t saved = Feature_IsOn(FEATURE_ID_BUZZER);

  Buzzer_SetEnabled(1U);
  if (Feature_IsOn(FEATURE_ID_BUZZER) == 0U)
  {
    Feature_Set(FEATURE_ID_BUZZER, saved);
    Buzzer_ApplySwitch();
    return 1;
  }

  Buzzer_SetEnabled(saved);
  return 0;
}

void Test_Init(void)
{
  s_count = 0U;

  (void)Test_Register("switch_table_sanity", Test_SwitchTableSanity);
#if FEATURE_BUZZER_ENABLED
  (void)Test_Register("buzzer_off_mutes_pb2", Test_BuzzerOffForcesMute);
  (void)Test_Register("buzzer_on_sets_flag", Test_BuzzerOnSetsFlag);
#endif
}

int Test_Register(const char *name, TestFn_t fn)
{
  if ((s_count >= TEST_MAX_CASES) || (name == (const char *)0) || (fn == (TestFn_t)0))
  {
    return -1;
  }

  s_cases[s_count].name = name;
  s_cases[s_count].fn   = fn;
  s_count++;
  return 0;
}

uint32_t Test_RunAll(void)
{
  uint32_t i;
  uint32_t pass = 0U;
  uint32_t fail = 0U;

  LOG_INFO("TEST BEGIN cases=%u", (unsigned int)s_count);

  for (i = 0U; i < s_count; i++)
  {
    int rc = s_cases[i].fn();
    g_test_last_rc = (uint32_t)rc;

    if (rc == 0)
    {
      pass++;
      LOG_INFO("TEST [%s] PASS", s_cases[i].name);
    }
    else
    {
      fail++;
      LOG_ERROR("TEST [%s] FAIL rc=%d", s_cases[i].name, rc);
    }
  }

  g_test_pass_count = pass;
  g_test_fail_count = fail;
  g_test_last_fail  = fail;

  LOG_INFO("TEST END pass=%u fail=%u", (unsigned int)pass, (unsigned int)fail);
  return fail;
}

void Test_Request(void)
{
  g_test_request = 1U;
}

void Test_Poll(void)
{
  if (g_test_request != 0U)
  {
    g_test_request = 0U;
    (void)Test_RunAll();
  }
}
