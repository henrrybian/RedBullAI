---
name: 阶段5-构建与J-Link单步调试
overview: 执行《STM32 蜂鸣器 PWM 全 AI 生成工作流（CodeBuddy IDE 版）》阶段 5：以实测绝对路径配置 CMake Debug 构建任务与 Cortex-Debug J-Link 主选调试配置（launch.json 逐字段经扩展 package.json 核实），就位一键烧录脚本与命令行 GDB 兜底产物，产出构建命令清单与连接排查决策树；F5 全链路（编译→连接→下载→停 main）经用户当轮授权实测验收通过，首次 GDB Server Timeout 按 T0+重试一次闭环；「共用 + 主选」一组验收全部满足，兜底保留未启用。
status: completed
completed_at: "2026-11-12"
archive_note: 本计划为归档补录。阶段 5 原会话因流程收尾未落盘计划文件；本文件依据 docs/stage5_report.md（阶段 5 验收报告，唯一证据源）与附录 B #25–29/#34 的实测结论补录，内容不超出报告已记录事实。
todos:
  - id: read-stage5-context
    content: 实际读取全部 @ 上下文（工作流 522–634 行、附录 B #25–29/#34、hardware_pins/env_check/build_commands 既有结论、阶段 0–4 归档计划与报告）
    status: completed
  - id: restate-scope-and-boundary
    content: 复述阶段 5 范围（只改配置不改源码）、调试路径双轨规则、烧录当轮授权边界与 J-Link FW 禁升级
    status: completed
  - id: probe-extension-config
    content: 探测本机 CodeBuddy 扩展实际配置位置与 schema（launch.json 字段逐条在 package.json 核实存在，路径取 env_check.md 8.3 实测值）
    status: completed
  - id: generate-build-task
    content: 生成 .vscode/tasks.json（CMake Debug Build：实测绝对路径 cmake --build build/Debug，preLaunchTask 依赖，默认构建组）
    status: completed
  - id: generate-launch-config
    content: 生成 .vscode/launch.json（cortex-debug + jlink servertype，device STM32F103ZE / swd / runToEntryPoint main / svdFile 实测路径 / armToolchainPath 实测路径）
    status: completed
  - id: confirm-flash-scripts
    content: 确认 scripts/flash.ps1 + flash.jlink（一键烧录）与 scripts/debug_gdb_cmds.txt（兜底命令行 GDB）已就位
    status: completed
  - id: generate-build-docs
    content: 生成 docs/build_commands.md（构建/清理/重建命令与实测证据、size 口径）与 docs/debug_guide.md（T0–T6 决策树、下载失败、断点不命中、optimized out 处置）
    status: completed
  - id: f5-full-chain
    content: B 条件执行（用户当轮授权后）：F5 一次完成编译→连接→下载→停 main；断点 Buzzer_Tick() 命中、F10 单步四变量变化、暂停态 SVD 外设视图可读、暂停停日志/恢复输出
    status: completed
  - id: timeout-handling
    content: 首次 F5 报 GDB Server Timeout：按 T0 查进程/端口 + 手工启动二进制验证 + 重试一次成功；登记「再遇同现象 T0+重试一次、连续 2 次失败切兜底」规则
    status: completed
  - id: archive-stage5-report
    content: 生成 docs/stage5_report.md（字段来源核对、异常处置记录、附录 B #25–29/#34 对照）并归档本计划
    status: completed
---

## 执行记录（2026-11-12）

- **共用①（构建）**：`cmake --build build/Debug`（tasks.json 实测路径）`BUILD_EXIT=0`、`0 error`；`.elf/.hex/.bin/.map` 四件齐全；`text 15764 / data 116 / bss 2100` → Flash 15,880 B（512K 的 3.0%）/ RAM 2,216 B（64K 的 3.4%）。证据：`docs/build_commands.md` 第 5 节。
- **共用②（烧录）**：`scripts/flash.ps1` + `flash.jlink` 就位；F5 下载链路实测成功；`BOOT0=0`（阶段 3 已闭环 PC=0x08004FE6）。
- **共用③（日志同步）**：断点暂停时串口停日志、F5 继续后恢复（用户实测确认）。
- **主选④（F5 全链路）**：首次 `Failed to launch jlink GDB Server: Timeout` → T0 查进程（无残留）/端口（全空闲）→ 手工启动 GDB Server 二进制 10 s 内存活 → **重试 F5 一次即成功**。GDB Server 实际参数 `-singlerun -nogui -if swd -port 50000 -swoport 50001 -telnetport 50002 -device STM32F103ZE`（扩展自动拼装，实测回显）。
- **主选⑤⑥⑦**：`Buzzer_LoadSegment(next, now);` 行断点稳定命中；F10 单步四变量（seg_index/freq/duty/remain）随步变化；暂停态 XPERIPHERALS 展开 `TIM3 @0x40000400` 全寄存器可读（TIM3 全 0 属正常，工程用 TIM2；运行态 `readMemory failed` 为扩展行为限制，仅暂停态观测）。
- **兜底路径**：`scripts/debug_gdb_cmds.txt` 保留**未启用**（主选可用，无需切换，附录 B #34 = N/A）。
- **边界**：只改配置，未改任何 C 源码/HAL/链接脚本/CMake 构建逻辑；未改跳线、未飞线、未升级 J-Link FW；烧录下载经用户当轮授权（F5）。

## 用户要求与阶段门

- 只执行阶段 5（工作流 522–634 行 + 附录 B #25–29/#34），阶段 6 验证闭环不越界。
- 调试配置路径与 J-Link 参数须先探测，禁止臆造；launch.json 每个字段在扩展 package.json 核实存在后才写入。
- 保持 SWD 可调试：不复用 PA13/PA14、不做 SWJ remap、不关 DBGMCU、Debug 保持 `-O0 -g3`；严禁 SWO/RTT。
- 烧录/下载动作须用户当轮显式授权；J-Link FW 升级弹窗一律取消。

## 关键决策（均有实测/核实依据）

1. **主选 = 图形化（Cortex-Debug）**：扩展已安装（阶段 0 实测 `{CB_EXT_AVAILABLE}`），字段可核实；兜底 = 命令行 GDB 保留不启用（文档 540 行规则：主选可用不切换）。
2. **launch.json 字段全部有出处**：`serverpath`/`svdFile`/`armToolchainPath` 取 `env_check.md` 8.3 实测绝对路径；`device`=`STM32F103ZE`、`interface`=`swd`、`runToEntryPoint`=`main` 显式写出；`showDevDebugOutput`=`none`（排查时可临时改 `raw`）。
3. **超时处置规则**：首次 GDB Server Timeout = 瞬时性（USB 枚举/安全软件扫描），处置 = T0 进程/端口检查 + 重试一次；不盲目改配置。
4. **外设视图使用规范**：一律断点命中（⏸）后打开/刷新；运行态报错可忽略。

## 交付物

| 文件 | 动作 |
| --- | --- |
| `.vscode/tasks.json` | 新增（CMake Debug Build 任务） |
| `.vscode/launch.json` | 新增（Cortex-Debug J-Link 主选配置） |
| `scripts/flash.ps1` / `scripts/flash.jlink` | 就位（一键烧录与下载校验） |
| `scripts/debug_gdb_cmds.txt` | 就位（兜底，未启用） |
| `docs/build_commands.md` | 新增完整文件 |
| `docs/debug_guide.md` | 新增完整文件 |
| `docs/stage5_report.md` | 新增完整文件 |
| 本计划 | 归档（补录），供阶段 6 `@` 引用 |

## 验收证据摘要（对照附录 B）

| # | 验收项 | 结论 | 证据 |
| --- | --- | --- | --- |
| 25 | F5 一次完成编译→连接→下载→停 main | ✅ | `stage5_report.md` 第 2.2/3.1 节 |
| 26 | `Buzzer_Tick()` 断点稳定命中 | ✅ | `stage5_report.md` 第 0 节⑤、第 4 节 |
| 27 | 单步可见四个状态变量变化 | ✅ | `stage5_report.md` 第 2.3 节 |
| 28 | SVD 外设寄存器视图可打开 | ✅ | `stage5_report.md` 第 2.4 节（暂停态） |
| 29 | 手工启动 GDB Server 可识别器件并 halt | ✅ | `stage5_report.md` 第 3.1 节排查③ |
| 34 | 主选不可用已切兜底且验收通过 | N/A | 主选可用，兜底保留未启用 |

**判定**：按工作流 621 行「共用 + 主选」一组全部满足 → **阶段 5 验收通过**（`stage5_report.md` 第 0 节）。

## 阶段 6 交接

- 可 `@` 引用：`docs/stage5_report.md`、`docs/build_commands.md`、`docs/debug_guide.md`、`docs/hardware_pins.md`、`docs/uart_wiring.md`、`docs/buzzer_sequence.md`、阶段 0–4 归档计划与本计划。
- 关键结论：F5 全链路可复现；外设寄存器观测仅暂停态有效；首次 GDB Server 超时按「T0 + 重试一次」处置；兜底 `scripts/debug_gdb_cmds.txt` 保留可作脱离图形化界面的交叉验证。
- 阶段 6 待办：按 `docs/acceptance_checklist.md` 第 1 节端到端清单执行（烧录仍需当轮授权），回填 `docs/delivery_report.md`，对照五条判定条件输出最终结论。
