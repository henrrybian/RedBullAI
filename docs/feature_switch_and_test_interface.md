# 功能开关与测试接口使用说明

> 目的：在不动既有阶段 0–5 成果的前提下，提供一个**编译期总开关 + 运行期开关接口 + 可注册测试接口**，方便逐步接入其他功能模块并单独测试。全部 AI 生成，用户零手写代码。

## 1. 两层开关，不要混淆

| 层次 | 位置 | 作用 | 何时用 |
| --- | --- | --- | --- |
| 编译期总开关 | `Core/Inc/feature_config.h` + `CMakeLists.txt` 的 `option()` | 决定某模块**是否编译进固件、是否上电初始化** | 长时间关闭蜂鸣器、专注测别的模块 |
| 运行期开关 | `Core/Inc/feature_switch.h` 的 `g_feature_state[]` | 决定模块**当前是否运行**，无需重新编译/烧录 | 调试时反复开合、GDB 里即时切换 |

一个模块占一个 `FEATURE_ID_xxx` 槽位，目前只有 `FEATURE_ID_BUZZER`。

## 2. 关闭蜂鸣器的两种方式

### 方式 A：编译期关闭（推荐用于长期切模块）

```powershell
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DFEATURE_BUZZER_ENABLED=OFF
cmake --build build/Debug
```

效果（已实测 `BUILD_EXIT=0`）：
- PB2 不配置、保持复位默认态；TIM2 更新中断不启动；
- 蜂鸣器相关主循环代码整段不编译，Flash 由 17,492 B 降到 15,208 B；
- 串口启动日志、测试接口照常工作。

需要恢复时去掉 `-DFEATURE_BUZZER_ENABLED=OFF` 或设为 `ON` 并重新配置。另有两个可选参数：
- `FEATURE_BUZZER_DEFAULT_ON=OFF`：仍编译蜂鸣器，但上电默认静音（运行期可再打开）；
- `FEATURE_TEST_ENABLED=OFF`：不编译测试框架。

### 方式 B：运行期关闭（调试时即时切换）

代码调用：

```c
Buzzer_SetEnabled(0U);   /* 关：PB2 立即常低、序列停止 */
Buzzer_SetEnabled(1U);   /* 开：未启动则从段1启动，否则从当前段恢复 */
Buzzer_ApplySwitch();    /* 仅当调试器直接改了状态时需要主动同步 */
```

调试器（断点停在主循环，Cortex-Debug 变量视图或 GDB 窗口）：

```text
p g_feature_state[FEATURE_ID_BUZZER]        # 查看：0=关 1=开
set g_feature_state[FEATURE_ID_BUZZER] = 0  # 关闭，下一轮主循环 ApplySwitch 生效
```

中断侧的 `Buzzer_IsrUpdateOutput()` 也会读同一个 volatile 状态，即使不走 `ApplySwitch`，关闭后 PB2 仍立即静音；中断内只有「读字节 + 比较 + 写 GPIO」，无函数调用、无 printf、无除法，保持阶段 4 硬约束。

## 3. 测试接口怎么用

三个内置用例（注册在 `test_interface.c`）：

| 用例名 | 验证内容 |
| --- | --- |
| `switch_table_sanity` | 开关表 `Set/Toggle/IsOn` 读写正确 |
| `buzzer_off_mutes_pb2` | 关闭蜂鸣器后 `GPIOB->ODR` bit2 = 0（PB2 常低） |
| `buzzer_on_sets_flag` | 打开蜂鸣器后开关位为 1 |

触发方式（二选一）：

```text
# 方式 1：调试器里请求（主循环 Test_Poll 自动执行）
set g_test_request = 1
# 继续运行，串口会打印 TEST BEGIN / 每条 PASS·FAIL / TEST END
```

```c
/* 方式 2：代码中请求 */
Test_Request();
```

结果从 USART1（115200-8N1）观察，同时可读 `g_test_pass_count` / `g_test_fail_count` / `g_test_last_fail` / `g_test_last_rc`。测试自身会保存并恢复原开关状态，不改变上电默认行为。

## 4. 接入一个新功能模块（标准三步）

1. 在 `feature_config.h` 追加一个编译期宏（照模板 `FEATURE_XXX_ENABLED`）；在 `CMakeLists.txt` 加对应 `option()` 并加入 `target_compile_definitions`。
2. 在 `feature_switch.h` 的 `FeatureId_t` 枚举里、`FEATURE_ID_COUNT` 之前追加 `FEATURE_ID_XXX`；在 `feature_switch.c` 的 `Feature_Init()` 里按新宏设置默认态。
3. 模块的源码里用 `Feature_IsOn(FEATURE_ID_XXX)` 判断运行态；要加测试就写一个 `static int MyTest(void)`（返回 0=PASS / 非 0=FAIL 码），在初始化中 `Test_Register("xxx_case", MyTest)` 注册，然后 `Test_Request()` 触发。

注册表上限 8 个用例（`TEST_MAX_CASES`）；用例运行在主循环上下文，禁止在其中调用阻塞延时或关闭中断，禁止在中断服务函数中调用测试接口。

## 5. 验证记录（2026-11-25）

| 配置 | 配置/构建 | Flash / RAM | 说明 |
| --- | --- | --- | --- |
| 默认（蜂鸣器 + 测试全开） | `CONFIG_EXIT=0` / `BUILD_EXIT=0`（23/23） | 17,492 B / 2,312 B | 链接期 nosys 桩与 RWX 段警告为阶段 2 已登记项 |
| `-DFEATURE_BUZZER_ENABLED=OFF` | `CFG_EXIT=0` / `BUILD_EXIT=0` | 15,208 B / 2,288 B | 蜂鸣器代码未参与链接，PB2 不配置 |

未做：未烧录、未接板、未做听感/ODR 电平实测——任何下载动作仍需用户当轮授权；内置用例的运行结果需上板后由用户回传串口日志判定。
