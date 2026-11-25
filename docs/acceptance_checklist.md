# 阶段 6：端到端验证与逐项验收勾选表

> 项目：STM32F103ZET6 红牛开发板，蜂鸣器 PWM 鸣叫（路径 A / TIM2 @10 kHz 软件 PWM），USART1 Debug，J-Link SWD 调试
> 生成日期：2026-11-12 ｜ 平台：Windows + PowerShell ｜ 开发前端：CodeBuddy IDE ｜ 用户零手写代码
> 依据：`STM32_Buzzer_PWM_Workflow.md` 阶段 6（654–720 行：验证点表 689–700、五条判定 704–710、回退矩阵 712–720）与附录 B #1–34；`@docs/hardware_pins.md`（锁定版）、`@docs/uart_wiring.md`、`@docs/buzzer_sequence.md`、`@docs/build_commands.md`、`@docs/debug_guide.md`、`@docs/stage5_report.md`。
> 本表勾选结果统一回填到 `docs/delivery_report.md` 后归档。

---

## 0. 执行前置（每次端到端验证会话开始前确认一次）

| # | 前置项 | 可观测确认方式 |
| --- | --- | --- |
| P1 | 烧录/下载动作（F5、`scripts/flash.ps1`）须**用户当轮显式授权**后才执行 | 用户在会话中明确回复「可以烧录/可以下载」 |
| P2 | J-Link 固件升级弹窗**一律取消**（V8 兼容/克隆类，严禁升级） | 弹窗截图，点 Cancel 后继续 |
| P3 | 外设寄存器观测（SVD 视图 / GDB 读寄存器）**仅暂停态有效** | 断点命中（⏸）后再展开 XPERIPHERALS；运行态 `readMemory failed` 提示可忽略（`stage5_report.md` 3.2） |
| P4 | 新开终端先补会话级 PATH（`build_commands.md` 第 1 节两行），或用第 1 节实测绝对路径调用 | `cmake --version` 输出版本号 |
| P5 | 禁忌自检：不带电插拔/改跳线、不用 SWO/RTT、不复用 PA13/PA14、不写 option bytes、不代杀用户进程 | 对照 `debug_guide.md` 第 9 节逐条目视 |

**仪器需求声明（硬约束 5）**：本项目**无示波器也能完成全部主要验收**。唯一「需仪器」项是附录 B #23 的原始波形测量（可选加分项）；其余项全部用「日志 + 变量观察 + `GPIOB->ODR` 电平佐证 + 用户听感回报」替代（阶段 4 已有先例，见 `buzzer_sequence.md` 第 4 节）。逐项标注见第 2 节「仪器/替代」列。

---

## 1. 端到端验证步骤清单（固定顺序，不可跳步；对照工作流 689–700 行）

> 每步的「预期可观测结果」均为可直接回传判定的证据：命令输出、日志文本、听感描述、截图。任何一步失败，按「失败时回到」列定位阶段，再按 `docs/troubleshooting.md` 第 1 节矩阵回退。

| # | 动作 | 预期可观测结果 | 仪器/替代 | 失败时回到 |
| --- | --- | --- | --- | --- |
| 1 | **断电静态检查** | 目视并拍照/文字回传：`JP5 BEEP` 短接、`JP6 JTAG_SEL`=SWD 档、`JP8 BOOT0`=GND 档（2-3，User Flash）、`JP9 BOOT1` 不把 PB2 强拉到固定电平；`CON1` DB9 接 **RS232-USB 线**（枚举 COM3，禁止 USB-TTL 直连）；J-Link 20pin 接 `J5`（pin1=Vref），与板共地 | 目视/照片（无需仪器） | **阶段 1**（`hardware_pins.md` 第 4 节 #1–4/#7/#8） |
| 2 | 上电 | 电源指示灯亮、无异常发热；PowerShell `Get-PnpDevice -Class Ports` 回显 `USB Serial Port (COM3)` `Status OK`；`Get-PnpDevice | Select-String J-Link` 回显 `VID_1366` `Status OK`（非 `CM_PROB_PHANTOM`）；GDB Server/F5 日志 `Target voltage: 3.31 V` | 日志/命令输出（无需仪器） | **阶段 0**（驱动/枚举）；接线类失败回**阶段 1** |
| 3 | 编译 | 内置终端执行会话级 PATH 两行后 `cmake --build build/Debug`：`BUILD_EXIT=0`、`0 error`（允许既有非 error 警告）；`Get-ChildItem build/Debug` 四件产物 `.elf/.hex/.bin/.map` 齐全；`arm-none-eabi-size` 输出 text/data/bss，Flash ≤512K、RAM ≤64K（基线 15,880 B / 2,216 B） | 命令输出（无需仪器） | **阶段 2**（CMakeLists/工具链/链接脚本）或 **阶段 5**（PATH/路径） |
| 4 | 烧录（**先取得用户当轮授权**，见 P1） | 主选 `F5`：preLaunchTask 编译 → DEBUG CONSOLE 打印 GDB Server 启动行（`-device STM32F103ZE -if swd -port 50000` 系）→ 下载完成 → **停在 `main()`**，无 error；兜底 `scripts/flash.ps1`：输出 `Erasing/Programming/Verifying ... Done.` 与 load size | 调试控制台/终端输出（无需仪器） | **阶段 5**（`debug_guide.md` T0–T6） |
| 5 | 复位看日志 | COM3 终端 `115200/8N1/None/CRLF`：每次复位 1 行启动日志 `[INF] STAGE3 FW=... SYSCLK=72000000Hz UART=USART1/115200-8N1 BEEP=PB2(pin48)/active-high PWM_PATH=A\r\n`；正文 148 B、无乱码无截断；复位瞬间 1 个前导杂散字节（`0xFC`/`0xF8`）判为 RS232 毛刺，不计入正文判定 | 终端文本回传（无需仪器） | **阶段 3**（接线/终端配置/printf 重定向） |
| 6 | 听鸣叫 + 段日志 | 用户听感回报：上电后「哔、哔、哔（三短）→ 停 0.6 s → 哔————（一长 0.8 s）→ 静音 2 s」循环（单循环 4150 ms）；COM3 每次段切换一条 `[INF] BUZZER seg=N/8 on=ON/OFF freq=XHz duty=Y% remain=Zms`，与 `buzzer_sequence.md` 第 2 节逐项一致；静音段确实无声 | 听感（用户回报）+ 终端日志（无需仪器） | **阶段 4**（有效电平/序列表/路径 A 语义） |
| 7 | 波形（本板无示波器，走替代项） | 四重替代（`buzzer_sequence.md` 第 4 节）：① 断点命中后变量视图读 `g_buzzer_seg_index/freq_hz/duty_pct/remain_ms` 与序列表逐段一致；② 段切换 INFO 日志参数一致；③ 暂停态读 `GPIOB->ODR`（`0x40010C0C`）bit2：响段=1（`0x14`）、停段=0（`0x10`）；④ 听感段时长与表偏差 ≤±50 ms（SysTick 粒度）。有示波器时才测 PB2 原始波形（响段 duty=100% 恒高 150/800 ms，停段恒低） | 无仪器即可（替代项 ①–④）；原始波形需示波器/逻辑分析仪（可选） | **阶段 4** |
| 8 | 单步调试（图形化主选） | `F5` 停在 `main` → 在 `buzzer.c` 的 `Buzzer_LoadSegment(next, now);` 行设断点 → F5 继续后**断点稳定命中** → `F10` 单步逐句推进；VARIABLES 视图四个状态变量随单步变化（`seg_index` 0→1→…→7→0 循环、`freq/duty` 响段 2/100 ↔ 停段 0/0、`remain_ms` 递减）；暂停态 XPERIPHERALS 展开 `TIM2 @0x40000000`：`PSC=0x47`(71)、`ARR=0x63`(99)，`USART1 BRR=0x271` | 调试控制台 + 变量/寄存器视图截图（无需仪器） | **阶段 5**（`debug_guide.md` 第 3–5 节）；主选连续 2 次失败切兜底 `scripts/debug_gdb_cmds.txt` |
| 9 | 日志与调试同步 | 断点暂停期间 COM3 **停止输出新日志**；F5 继续后下一条段切换 INFO 出现 | 终端 + 调试视图对照（无需仪器） | **阶段 3 / 4** |
| 10 | 回归 | 连续复位（或断电重上电）3 次：启动日志正文逐字一致、鸣叫序列行为一致；`{BUZZER_TYPE}`=有源 / `{BUZZER_ACTIVE_LEVEL}`=高 / `{PWM_PATH}`=A 三项分支语义复核无误（日志摘要行逐字核对） | 终端文本 ×3 + 听感回报（无需仪器） | **阶段 4** |

---

## 2. 逐项验收勾选表（对照工作流附录 B #1–34）

> 「证据来源」列 = 已归档的实测证据文档；本次端到端复验产生的新证据统一回填 `docs/delivery_report.md` 第 2 节并在本表勾选。
> 勾选口径：✅ 已有归档证据且本次复验通过；🔁 归档已有、本次复验待回填；☐ 本次新产生待回填。

| # | 阶段 | 验收项 | 可观测证据 | 实测证据来源文档 | 仪器/替代 | 勾选 |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | 0 | cmake / ninja / arm-none-eabi-gcc 有版本号 | `--version` 输出 | `docs/env_check.md` 8.3 | 无需 | 🔁 |
| 2 | 0 | CodeBuddy 环境可用（Plan/内置终端/@ 引用） | 终端输出 + 界面截图 | 阶段 0 归档计划 | 无需 | 🔁 |
| 3 | 0 | J-Link 与 SVD 路径已定位 | 绝对路径记录 | `docs/env_check.md` 8.3 | 无需 | 🔁 |
| 4 | 1 | 映射表字段均有证据页码与置信度 | 映射表本体 | `docs/hardware_pins.md` 第 1–2 节 | 无需 | 🔁 |
| 5 | 1 | 蜂鸣器类型已实测（有源） | PB2 恒高持续鸣叫 + 用户回报 | `docs/hardware_pins.md` 第 4 节 #5、附录 B.2 | 用户听感回报 | 🔁 |
| 6 | 1 | 有效电平已实测（高电平鸣叫） | PB2=1/0 听感对比 + `ODR 0x14/0x10` | `docs/hardware_pins.md` 第 4 节 #6、附录 B.2 | 用户听感回报 | 🔁 |
| 7 | 1 | 串口电平方式已确认（RS232-USB → COM3） | 接线记录 | `docs/hardware_pins.md` 第 4 节 #7；`docs/uart_wiring.md` 第 0/2 节 | 目视 | 🔁 |
| 8 | 1 | `{PWM_PATH}` 已选定（A） | 映射表字段 | `docs/hardware_pins.md` 第 5 节 | 无需 | 🔁 |
| 9 | 1 | JP5/JP6/JP8/JP9 状态已核对 | 跳线帽照片/目视记录；BOOT0=2-3 由 halt PC 实测闭环 | `docs/hardware_pins.md` 第 4 节 #1–4；`docs/uart_wiring.md` 第 0 节边界声明 | 目视（断电） | 🔁（步骤 1 逐项复验） |
| 10 | 2 | 编译 0 error | `cmake --build` 输出 `BUILD_EXIT=0` | `docs/build_commands.md` 第 5 节 | 无需 | 🔁 |
| 11 | 2 | `.elf/.hex/.bin` 齐全 | 文件列表（含 .map 四件） | `docs/build_commands.md` 第 3/5 节 | 无需 | 🔁 |
| 12 | 2 | Flash/RAM 占用在容量内 | `arm-none-eabi-size`：Flash 15,880 B（512K 的 3.0%）/ RAM 2,216 B（64K 的 3.4%） | `docs/build_commands.md` 第 3.1/5 节 | 无需 | 🔁 |
| 13 | 2 | `SystemCoreClock` = 72 MHz | 启动日志 `SYSCLK=72000000Hz`（运行态 `HAL_RCC_GetSysClockFreq()`） | `docs/uart_wiring.md` 第 4 节；`docs/stage3_report.md` 第 5 节 | 无需 | 🔁 |
| 14 | 2 | SWD 引脚未被复用 | Core/** 无 PA13/PA14 配置、无 SWJ remap、无 DBGMCU 关闭（静态自检 0 命中） | `docs/debug_guide.md` 第 1 节 T4；阶段 2/3/4 归档计划自检 | 无需 | 🔁 |
| 15 | 3 | 复位后稳定输出启动日志 | COM3 3 次复位 `LEN=149`、正文逐字一致 | `docs/uart_wiring.md` 第 4 节；`docs/stage3_report.md` 第 6.3 节 | 无需 | 🔁 |
| 16 | 3 | 日志无乱码、无截断、无刷屏 | Latin1 逐字节捕获正文 148 B 无杂散字节；仅复位 1 行 | `docs/uart_wiring.md` 第 4 节（前导 `0xFC` 毛刺单独登记，不计入正文） | 无需 | 🔁 |
| 17 | 3 | 中断内无 printf | TIM2 更新中断与所有 ISR 无打印（代码自检 + 单步暂停无日志佐证） | `docs/uart_wiring.md` 第 3 节；`docs/buzzer_sequence.md` 第 2 节 | 无需 | 🔁 |
| 18 | 3 | 单步执行与日志顺序一致 | `main.c:171` 断点：停住仅 `0xFC`，`next` 后 148 B 日志出现 | `docs/stage3_report.md` 第 5/6.3 节 | 无需 | 🔁 |
| 19 | 4 | 蜂鸣器按序列表鸣叫 | 用户回报「三短→停 0.6s→一长 0.8s→静音 2s」与表逐段一致 | 阶段 4 归档计划（用户回报原文）；`docs/stage4_report.md` | 用户听感回报 | 🔁（步骤 6 复验） |
| 20 | 4 | 静音段确实无声 | 静音段无听感 + 暂停态 `ODR` bit2=0（`0x10`） | 阶段 4 归档计划（⑤ + ODR 证据互证） | 用户听感 + 寄存器替代 | 🔁 |
| 21 | 4 | 状态切换输出一条 INFO 日志 | `[INF] BUZZER seg=N/8 ...` 每段 1 条、无刷屏 | 阶段 4 归档计划（⑥ 1711 B 捕获）；`docs/stage4_report.md` | 无需 | 🔁 |
| 22 | 4 | 主循环无 `HAL_Delay`、无忙等 | `Buzzer_Tick()` 基于 `HAL_GetTick()` O(1)；日志不阻塞单步 | `docs/buzzer_sequence.md` 第 1 节；阶段 4 归档计划职责二分 | 无需 | 🔁 |
| 23 | 4 | 波形频率与占空比符合设定 | **替代路径（无示波器）**：变量四件 + 段日志 + `ODR` 电平 + 听感时序（`buzzer_sequence.md` 第 4 节四项）；原始波形需示波器（可选加分） | `docs/buzzer_sequence.md` 第 3–4 节；阶段 4 归档计划 ⑦ | 变量+日志替代；原始波形需仪器 | 🔁（替代项） |
| 24 | 4 | 有源蜂鸣器未生成变频变音调逻辑 | 代码语义核对：`freq_hz` 仅表达通断节奏、无变频调用；固件无任何变调路径 | `docs/buzzer_sequence.md` 第 1 节；阶段 4 归档计划硬约束 | 无需 | 🔁 |
| 25 | 5 | F5 一次完成编译→连接→下载→停 main | DEBUG CONSOLE 无 error、停在 `main`；首次 Timeout 已按 T0+重试一次闭环 | `docs/stage5_report.md` 第 2.2/3.1 节 | 无需 | 🔁（步骤 4 复验） |
| 26 | 5 | `Buzzer_Tick()` 断点稳定命中 | 调试控制台断点命中记录 | `docs/stage5_report.md` 第 0 节⑤、第 4 节#26 | 无需 | 🔁（步骤 8 复验） |
| 27 | 5 | 单步可见四个状态变量变化 | VARIABLES 视图逐值变化 | `docs/stage5_report.md` 第 2.3 节 | 无需 | 🔁 |
| 28 | 5 | SVD 外设寄存器视图可打开 | 暂停态 `TIM3 @0x40000400` 全寄存器可读（TIM3 全 0 属正常，本工程用 TIM2） | `docs/stage5_report.md` 第 2.4 节；3.2 节（仅暂停态有效） | 暂停态寄存器读（无需仪器） | 🔁 |
| 29 | 5 | 手工启动 GDB Server 可识别器件并 halt | 终端回显启动设置 + `Target voltage: 3.31 V` + F5 停 main 实证 | `docs/stage5_report.md` 第 3.1 节排查③ | 无需 | 🔁 |
| 30 | 6 | 连续复位 3 次行为一致 | 日志 ×3 逐字一致 + 听感一致 | **本次端到端复验产生** → `docs/delivery_report.md` 第 2 节 | 无需 | ☐ |
| 31 | 6 | 全程零手写代码 | 交互记录：所有代码/配置/脚本均 AI 生成 | 阶段 0–5 归档计划 + 本次会话记录 | 无需 | ☐（报告第 4 节确认） |
| 32 | 6 | 交付完成报告已回填实测结果 | 本文档 + `docs/delivery_report.md` 回填归档 | `docs/delivery_report.md` | 无需 | ☐ |
| 33 | 0–6 | 每阶段一个独立 Plan，归档于 `.codebuddy/plans/` | 计划列表截图/文件清单 | `.codebuddy/plans/` 阶段 0–5 五份归档 | 无需 | 🔁 |
| 34 | 5 | 主选不可用已按规则切兜底且验收通过 | 路径选择结论 + 终端输出 | `docs/stage5_report.md` 第 4 节#34：N/A（主选可用，`scripts/debug_gdb_cmds.txt` 保留未启用） | 无需 | 🔁（N/A） |

---

## 3. 「验收通过」判定条件（对照工作流 704–710 行，五条必须同时满足）

| # | 判定条件 | 本项目可观测判据 | 判定来源 |
| --- | --- | --- | --- |
| 1 | 编译 `0 error`，`.elf/.hex/.bin` 齐全，size 在 `{MCU}` 容量内 | `BUILD_EXIT=0`；四件产物；Flash ≤512K / RAM ≤64K | 步骤 3 + `build_commands.md` 第 5 节 |
| 2 | 串口终端稳定输出启动日志与段切换日志，无乱码、无刷屏，`SYSCLK` 数值正确 | 正文 148 B 逐字一致、`SYSCLK=72000000Hz`、每段仅 1 条 INFO（前导 `0xFC`/`0xF8` 复位毛刺不计入正文） | 步骤 5/6 + `uart_wiring.md` 第 4 节 |
| 3 | 蜂鸣器按序列表鸣叫，且行为与 `{BUZZER_TYPE}`=有源 / `{BUZZER_ACTIVE_LEVEL}`=高电平 / `{PWM_PATH}`=A 三个实测锁定项一致 | 听感与 `buzzer_sequence.md` 第 2 节逐段一致；`ODR` 响=高/停=低未反相；无变频变调逻辑 | 步骤 6/7 + `hardware_pins.md` 50–52、78 行 |
| 4 | J-Link SWD 可连接、可下载、断点可命中；单步可观察状态变量（图形化主选变量视图）；「共用 + 主选」一组验收全部满足 | F5 停 main、`Buzzer_Tick()` 断点命中、四变量变化、暂停态 SVD 可读 | 步骤 4/8/9 + `stage5_report.md` 第 0 节 |
| 5 | 用户全程没有手写任何代码，所有代码与配置均由 AI 生成；交付完成报告已回填实测结果 | 会话交互记录 + `docs/delivery_report.md` 回填归档 | 报告第 4 节 + 勾选表 #31/#32 |

**五条全部 ✅ 才输出「验收通过」**；任一条不满足，按 `docs/troubleshooting.md` 第 1 节回退矩阵定位阶段处置后重走本清单第 1–10 步受影响的后续步骤。
