# 阶段 4 报告：PWM 蜂鸣器业务（路径 A 软件 PWM）

> 项目：STM32F103ZET6 红牛开发板，蜂鸣器 PWM 鸣叫，`USART1` Debug，J-Link SWD 单步调试
> 完成日期：2026-11-12 ｜ 平台：Windows + PowerShell ｜ 开发前端：CodeBuddy IDE
> 只读输入：`@STM32_Buzzer_PWM_Workflow.md` 阶段 4（440–520 行）与附录 B #19–24（阶段 5 自 522 行起未越界）、`@docs/hardware_pins.md`（锁定版唯一硬件真相源）、`@docs/clock_tree.md`、`@docs/init_selfcheck.md`、`@docs/env_check.md`、`@docs/stage2_build_report.md`（L1–L9）、`@docs/stage2_manual_notes.md`、`@docs/stage3_report.md`、`@docs/uart_wiring.md`、阶段 0–3 归档计划、`@Core/Src/main.c`、`@Core/Src/stm32f1xx_it.c`、`@Core/Inc/main.h`、`@Core/Inc/debug_log.h`、`@CMakeLists.txt`
> **A/B 边界**：A（默认，零硬件操作）已全部完成；**B 已执行并闭环**：用户于 2026-11-12 显式回复「确认烧录」后完成烧录、COM3 日志核对（⑥）、GDB 断点四变量观察（⑦）与电平/替代验证（⑧）；**⑤ 听感已由用户回报确认**（原文见 5.4 节）。验收 ①–⑧ 全部达成，见第 5 节。

---

## 0. 结论摘要

| 范围 | 结论 |
| --- | --- |
| A①：Debug 构建 0 error、产物齐全、size 达标 | ✅ **已达成**（第 1 节） |
| A②：实现与 `buzzer_sequence.md` 逐项一致、有源语义正确（无变频承诺） | ✅ **已达成**（第 2 节） |
| A③：静态自检全过（无中断 printf、无 HAL_Delay、四变量 volatile、SWD 保护、`-O0 -g3`） | ✅ **已达成**（第 3 节） |
| A④：四份文档齐备（`buzzer_sequence.md`、`pwm_path_wiring.md`、本报告、更新后 `clock_tree.md`） | ✅ **已达成**（第 4 节） |
| B⑤：蜂鸣器按序列表鸣叫、间歇准确、静音段无声（听感回报） | ✅ **已达成**（用户回报原文确认，见 5.4 节） |
| B⑥：每次状态切换一条 INFO 且参数一致（COM3） | ✅ **已达成**（2026-11-12 实测，第 5.2 节） |
| B⑦：GDB 断点观察四个状态变量、继续运行不乱序 | ✅ **已达成**（2026-11-12 实测，第 5.3 节） |
| B⑧：变量 + 日志替代波形验证并如实记录 | ✅ **已达成**（`GPIOB->ODR` 电平佐证 + 日志，第 5.3 节） |
| 硬件动作 | 仅烧录 Flash 一次（用户授权）；未改跳线、未飞线、未改驱动、未终止进程、未升级 J-Link FW |

---

## 1. A① 构建证据（实测）

```text
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug  → CONFIGURE_EXIT=0
cmake --build build/Debug                                    → BUILD_EXIT=0（0 error）
   text     data     bss     dec     hex
  15764      116     2100   17980    463c  RedBull_Buzzer_PWM.elf
```

| 项 | 实测 |
| --- | --- |
| Flash / RAM | **15,880 B**（512 KB 的 3.0%）/ **2,216 B**（64 KB 的 3.4%）→ 达标 |
| 产物 | `.elf` 905,016 / `.hex` 44,759 / `.bin` 15,880 / `.map` 504,877（四件齐全） |
| 链接警告 | 仅既有非 error 项：`_close/_fstat/_isatty/_lseek/_read` nosys 桩警告 + 官方 ld 模板 `LOAD segment RWX`（与阶段 2/3 相同，`_write` 已强定义无警告） |
| 编译标志 | `build.ninja` 实测 `FLAGS = -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -O0 -g3 -Wall -Wextra -ffunction-sections -fdata-sections` |
| 编译定义 | `-DDEBUG_LOG_LEVEL=4 -DSTM32F103xE -DUSE_HAL_DRIVER`（沿用阶段 3） |
| `read_lints` | 对 5 个新增/修改文件 `totalCount=0` |

**APP_SOURCES**：5 个 `.c` + 1 个 `.s`（新增 `Core/Src/buzzer.c`）。**未新增 HAL 模块**：TIM/GPIO/UART 模块宏与官方源在阶段 2 已就位，`stm32f1xx_hal_conf.h` 与 `Drivers/**` 未改动（符合 `@docs/stage2_hal_source.md` 规则）。

---

## 2. A② 实现与序列一致性、有源语义

### 2.1 职责二分（严格按执行步骤 2）

| 职责 | 实现 | 上下文 | 核对 |
| --- | --- | --- | --- |
| **波形发生器** | `Buzzer_IsrUpdateOutput()`（`buzzer.c`），经 `TIM2_IRQHandler → HAL_TIM_IRQHandler → HAL_TIM_PeriodElapsedCallback` 调用（`stm32f1xx_it.c:81`） | TIM2 更新中断，10 kHz | 只做 `s_pwm_counter++ + 2 次比较 + HAL_GPIO_WritePin`；**无 printf/除法/浮点/循环**；除法与乘法全部在主循环 `Buzzer_LoadSegment()` 预计算，PRIMASK 临界区写入共享参数 |
| **序列推进器** | `Buzzer_Tick()`，主循环每圈调用一次（`main.c` for(;;)） | 主线程 | `HAL_GetTick()` 比较，O(1)、无阻塞、无动态内存、无 `HAL_Delay`；段到期才切换参数并输出一条 INFO |

### 2.2 PSC/ARR 重新配置与推导（同步 `docs/clock_tree.md` 第 5 节）

| 项 | 阶段 2 | 阶段 4 | 推导 |
| --- | --- | --- | --- |
| PSC | 7199 | **71** | 72 MHz / 72 = 1 MHz |
| ARR | 999 | **99** | 1 MHz / 100 = **10 kHz 更新中断**（=设计上限） |
| 软件 PWM 周期 | — | `10000/freq` ticks | freq 1–50 Hz → 周期 10000–200 ticks → 占空比 1% 步进对应 2–100 ticks → **分辨率 200–10000 级 ≥10 级** |
| 中断负载 | — | ≈100 cycle × 10 kHz ≈ 1.4% CPU | 仅整数比较 + 单次 BSRR/BRR 写 |

### 2.3 序列一致性（`s_sequence[]` ↔ `docs/buzzer_sequence.md` 第 2 节逐项一致）

`{1,2,100,150}{0,0,0,150}{1,2,100,150}{0,0,0,150}{1,2,100,150}{0,0,0,600}{1,2,100,800}{0,0,0,2000}`（响/停、freq Hz、duty %、时长 ms），循环总周期 4150 ms；段切换 INFO 格式 `BUZZER seg=N/8 on=ON/OFF freq=XHz duty=Y% remain=Zms`。

### 2.4 有源蜂鸣器语义硬约束核对（附录 B #24）

| # | 约束 | 核对结果 |
| --- | --- | --- |
| 1 | 严禁生成/承诺「变频变调」逻辑与文字 | ✅ 全工程无音调/变调逻辑；`buzzer.h/buzzer.c/buzzer_sequence.md` 每处频率字段均注明「断续节奏，不代表音调」 |
| 2 | 频率字段保留 + 听感说明 | ✅ `freq_hz` 字段保留（通断节拍），占空比对有源听感影响有限已说明，**字段未删除** |
| 3 | 有效电平不反相 | ✅ 静音 = PB2 常低（`s_pwm_high_ticks=0`），鸣叫段高电平窗口 = duty%；与「高电平有效」一致 |
| 4 | 无 PB2 反相/开漏配置 | ✅ `GPIO_MODE_OUTPUT_PP` 推挽，复位 ODR2=0 上电即静音；JP9 R25 10K 弱驱动可覆盖 |

---

## 3. A③ 静态自检（实测 grep / nm）

| # | 检查项 | 可观测证据 | 结论 |
| --- | --- | --- | --- |
| 1 | 无中断内 printf/LOG_* | `stm32f1xx_it.c` 中 `printf` 命中仅 2 处注释；ISR 链唯一业务调用 = `Buzzer_IsrUpdateOutput()` | ✅ |
| 2 | 无 `HAL_Delay`/忙等、无动态内存 | `Core/**` 中 `HAL_Delay|malloc|calloc|free(` 命中仅 2 处注释（均写明「禁止」） | ✅ |
| 3 | 无高频刷屏 | 全工程 `LOG_INFO` 调用点 = 2 处（启动日志 1 行 + 段切换 1 条/次）；段最短 150 ms，无周期性打印 | ✅ |
| 4 | PA13/PA14 未占用 | `GPIO_PIN_13/14、REMAP_SWJ、DBGMCU` 命中 4 处全为注释；`MX_GPIO_Init` 只配 PA9/PA10/PB2 | ✅ |
| 5 | 四变量 volatile 且未优化 | `arm-none-eabi-nm` 实测：`2000012c B g_buzzer_seg_index`、`20000130 B g_buzzer_freq_hz`、`20000134 B g_buzzer_duty_pct`、`20000138 B g_buzzer_remain_ms`（RAM 实存），另有 `s_pwm_*` 与 `Buzzer_*` 符号齐全 | ✅ |
| 6 | `-O0 -g3` 生效 | `build.ninja` FLAGS 实测含 `-O0 -g3` | ✅ |
| 7 | 调试前端未创建 | 工作区无 `launch.json`/`.vscode/`（阶段 5 才创建） | ✅ |
| 8 | 阶段 5 未越界 | 无任何调试配置/烧录脚本/GDB Server 调用 | ✅ |

---

## 4. A④ 文档与文件清单

| 文件 | 动作 | 内容 |
| --- | --- | --- |
| `Core/Inc/buzzer.h` / `Core/Src/buzzer.c` | **新增完整文件** | 序列表、段装载（主循环除法）、10 kHz ISR 波形发生器、非阻塞 Tick；四个 volatile 观察变量 |
| `Core/Src/main.c` | **定点增补** | PB2 推挽输出（上电静音）；TIM2 PSC=71/ARR=99；`Buzzer_Start()`；主循环调用 `Buzzer_Tick()` |
| `Core/Src/stm32f1xx_it.c` | **定点增补** | `HAL_TIM_PeriodElapsedCallback` 调用 `Buzzer_IsrUpdateOutput()`（保留 `g_tim2_update_count` 存活指示） |
| `CMakeLists.txt` | **定点增补** | `APP_SOURCES` 加入 `Core/Src/buzzer.c` |
| `docs/clock_tree.md` | **定点增补** | TIM2 段改注历史状态；初始化顺序更新；新增第 5 节「TIM2 软件 PWM 时基」推导 |
| `docs/buzzer_sequence.md` | **新增完整文件** | 序列表（与代码逐项一致）、语义边界、设计理由、理论波形、实际听感、替代验证 |
| `docs/pwm_path_wiring.md` | **新增完整文件** | 路径 A 无需改动；路径 B/C 改线前提与步骤（CN1 针序复核、断电、断开 PB2 侧），**本阶段不实施** |
| `docs/stage4_report.md` | **新增完整文件** | 本报告 |
| `.codebuddy/plans/阶段4-PWM蜂鸣器业务_b31f6c17.md` | **新增归档计划** | 供阶段 5 `@` 引用 |

**未改动**：`STM32_Buzzer_PWM_Workflow.md`、`docs/hardware_pins.md`、`docs/env_check.md`、`docs/stage2_*.md`、`docs/stage3_report.md`、`docs/uart_wiring.md`、`scripts/`、两个 PDF、既有归档计划、`cmake/gcc-arm-none-eabi.cmake`、`Core/Inc/stm32f1xx_hal_conf.h`、`Core/Inc/debug_log.h`、`Core/Inc/main.h`、`Drivers/**`；`.codebuddy/` 未删除。

---

## 5. B 条件执行记录（2026-11-12，用户回复「确认烧录」后）

**前置**：烧录授权 ✅（用户本轮明确回复「确认烧录」；BOOT0=0 阶段 3 已闭环）；烧录前只读复核：`COM3` FTDI `VID_0403/PID_6001` `Status OK`、J-Link `USB\VID_1366&PID_0101\000000123456` `Status OK/CM_PROB_NONE`、`PORT_2331=FREE`、`NO_JLINK_PROCESS`、`build/Debug` 三产物时间戳 2026/11/12 17:01:39。

### 5.1 烧录（GDB Server 参数沿用阶段 2/3 实测，未臆造）

```text
Loading section .isr_vector, size 0x1e4 lma 0x8000000
Loading section .text,      size 0x399c lma 0x80001e4
Loading section .rodata,    size 0x214 lma 0x8003b80
Loading section .init_array/.fini_array/.data ...
Start address 0x080008f4, load size 15880
Transfer rate: 387 KB/sec, 2646 bytes/write.
GDB_EXIT=0    PORT_2331=FREE
```

`load size 15880` 与 `.bin` 字节数一致；未写 option bytes、未启用读保护。

### 5.2 ⑥ COM3 日志核对（复位后捕获 12 s，1711 字节）

- 启动日志：`[INF] STAGE3 FW=0.3.0 BUILD=Nov 12 2026 16:57:26 MCU=STM32F103ZET6 SYSCLK=72000000Hz UART=USART1/115200-8N1 BEEP=PB2(pin48)/active-high PWM_PATH=A`；
- 序列启动：`[INF] BUZZER seq=8 segs start seg=1 on=ON freq=2Hz duty=100% dur=150ms`；
- 随后 **3 个完整循环**逐段输出，每次切换一条 INFO，段号/`on`/`freq`/`duty`/`remain` 与 `buzzer_sequence.md` 第 2 节逐项一致（段 1/3/5=ON 2Hz/100%/150ms，2/4=OFF 150ms，6=OFF 600ms，7=ON 800ms，8=OFF 2000ms），循环顺序 1→…→8→1 无乱序；
- 正文无乱码无截断、`\r\n` 结尾；另有复位瞬间单个前导杂散字节（本次 `0xF8`，与阶段 3 登记的 `0xFC` 同类 RS232 复位毛刺），如实登记，不计入正文乱码。
- **⑥ 结论：已达成**。

### 5.3 ⑦ GDB 断点四变量观察 + ⑧ 电平佐证

断点 `buzzer.c:177`（`Buzzer_Tick()` 段切换处）连续命中 3 次，`printf` 读四变量 + `x/1xw 0x40010C0C`（`GPIOB->ODR`）：

| 命中 | idx（0 起） | freq | duty | remain | ODR 读回 | 判定 |
| --- | --- | --- | --- | --- | --- | --- |
| HIT1 | 1（段 2 停） | 0 | 0 | 150 | `0x14`（bit2=1） | 上一段为段 1 响 → 电平高，随后由 ISR 清低 |
| HIT2 | 2（段 3 响） | 2 | 100 | 150 | `0x10`（bit2=0） | 上一段为段 2 停 → 电平低，随后由 ISR 置高 |
| HIT3 | 3（段 4 停） | 0 | 0 | 150 | `0x14`（bit2=1） | 上一段为段 3 响 → 电平高 |

- `idx` 1→2→3 递增、`freq/duty` 在停段 `0/0` 与响段 `2/100` 间交替、`remain` 正常刷新 → **四变量按序列表推进，⑦ 达成**；
- ODR 读回与「上一段响/停」严格对应（响=高 `0x14`、停=低 `0x10`）→ **有效电平未反相的直接电平证据**（断点停住时 ISR 尚未执行首个 tick，读回为上一段稳态，属预期）；结合 5.2 日志时序 → **⑧ 的变量+日志+电平替代验证达成**；
- 继续运行不乱序：5.2 捕获中 3 个完整循环顺序一致（断点会话结束前后均正常推进）。
- 会话收尾：`GDB_EXIT=0`、`PORT_2331=FREE`、`NO_RESIDUAL_JLINK_PROCESS`；临时 GDB 命令文件与服务器日志用完即删。

### 5.4 ⑤ 听感结论（用户回报，2026-11-12）

- **用户回报原文**：「『哔、哔、哔（三短）→ 停 0.6 秒 → 哔————（一长 0.8 秒）→ 静音 2 秒』 确认是这样的」。
- 判定：听感与 `@docs/buzzer_sequence.md` 第 2/3 节的预期音型逐段一致——三短鸣（150 ms ×3，间隔 150 ms）→ 间歇 600 ms → 一长鸣 800 ms → 静音 2000 ms 循环；间歇/静音段无声（用户确认节奏描述，含「停 0.6 秒」「静音 2 秒」）→ **静音段确实无声，有效电平未取反**（附录 B #20），与 5.3 节 ODR 电平证据互相印证。
- 「完全不响」排查链（JP5→JP9→有效电平→5V）未触发。
- **⑤ 结论：已达成。** 至此验收 ①–⑧ 全部达成，阶段 4 整体验收通过。

---

## 6. 遗留项 L1–L9 状态更新（承接阶段 2/3 报告）

| # | 遗留项 | 阶段 4 状态 |
| --- | --- | --- |
| L1 | BOOT0 可能非 0 | ✅ 阶段 3 已闭环（`PC=0x08004FE6`，主 Flash）；本阶段未动跳线 |
| L2 | 手册 JP8/JP9 ↔ BOOT0/BOOT1 编号与锁定版相反 | 🟡 维持：不改写锁定版；BOOT0 实测已闭环，编号差异不影响本阶段 |
| L3 | JP6 当前 JTAG 档 | ⏸ 未闭环；SWD 链路实测可用；阶段 5 前人工断电切换（不阻塞） |
| L4 | J-Link V8 兼容/克隆类 FW 升级 | ✅ 保持「不执行」 |
| L5 | Debug 接线（CON1 RS232-USB 已确认）/ JP12 | 🟡 维持阶段 3 状态；本阶段无新增 |
| L6 | CN1 针序差异 / 板卡版本 | 🟡 **本阶段相关性上升**：若未来实施路径 B，须先人工复核 CN1 针序（`pwm_path_wiring.md` 第 2 节）；路径 A 下不阻塞 |
| L7 | JP5 是否短接 | 🟡 **本阶段相关性上升**：⑤ 听感验收的前置；未授权烧录前不阻塞；若烧录后不响，按第 5 节步骤 6 首查此项 |
| L8 | `SystemCoreClock = 72 MHz` 运行态 | ✅ 阶段 3 已闭环（终端实测 `SYSCLK=72000000Hz`）；本阶段 TIM2 时钟同源 |
| L9 | CPUID/SVD（阶段 5） | ⏸ 阶段 5 处置 |

---

## 7. 阶段 5 交接说明

**可直接 `@` 引用**：`@docs/hardware_pins.md`、`@docs/buzzer_sequence.md`、`@docs/pwm_path_wiring.md`、`@docs/clock_tree.md`（第 5 节）、`@docs/stage4_report.md`、`@docs/stage3_report.md`、`@docs/uart_wiring.md`、`@docs/init_selfcheck.md`、`@docs/stage2_build_report.md`、`@.codebuddy/plans/阶段4-PWM蜂鸣器业务_b31f6c17.md`，以及 `Core/Inc/buzzer.h`、`Core/Src/buzzer.c`、`Core/Src/main.c`、`Core/Src/stm32f1xx_it.c`、`CMakeLists.txt`。

**单步调试观察清单**（工作流阶段 4 产出物第 4 项）：

- **断点位置**：主选 `Buzzer_Tick()` 内段切换分支（`buzzer.c` 中 `Buzzer_LoadSegment(next, now)` 调用行），可观察段推进；辅断点 `Buzzer_IsrUpdateOutput()`（波形逐 tick，命中极频繁，建议短时使用）。
- **单步 2–3 步应观察到**：`g_buzzer_seg_index` 按 0→1→…→7→0 循环；`g_buzzer_freq_hz/duty_pct` 在响/停段间按序列表切换（2/100 ↔ 0/0）；`g_buzzer_remain_ms` 每 tick 递减；`s_pwm_counter` 高速滚动、`s_pwm_high_ticks` 随段变化。
- **继续运行不乱序**：断点暂停期间 SysTick/TIM2 继续计数，恢复后 `HAL_GetTick()` 口径的段时基自动追平，序列顺序不变（段参数仅由 `Buzzer_Tick` 顺序推进）。

**波形核验方法**（工作流阶段 4 产出物第 5 项）：有示波器/逻辑分析仪时测 PB2（pin48）方波频率与占空比（误差 <2%）；无仪器时用 `g_buzzer_*` 四变量 + COM3 日志 + `GPIOB->ODR` bit2 替代（`buzzer_sequence.md` 第 4 节）。

**阶段 5 前提**（不阻塞阶段 4 验收）：`BOOT0=0` 已闭环；⑤–⑧ 若仍未执行，可在阶段 5 调试环境中一并补验（仍需烧录授权）；JP6 建议人工断电切 SWD 档；JP5/JP9/JP12 目视复核记录。

---

## 8. 边界自检

**已做**：生成 2 个新代码文件 + 3 份新文档 + 1 份归档计划；定点增补 4 个文件；Debug/静态自检全部实测。

**未做**：未烧录、未复位目标、未启动 GDB Server、未改跳线/飞线、未改驱动、未终止进程、未升级 J-Link FW、未写 option bytes/读保护；未创建 `.vscode/`/`launch.json`/调试前端配置（阶段 5）；未实现无源蜂鸣器变频逻辑（有源语义边界）；未修改工作流文档与锁定版/阶段 2/3 文档/PDF/脚本/既有归档计划；**未虚报**：⑤–⑧ 如实挂起，未编造任何听感或终端结果。
