# 阶段 5 报告：CMake 构建与 CodeBuddy 中的 J-Link 单步调试（主选图形化路径）

> 项目：STM32F103ZET6 红牛开发板，蜂鸣器 PWM 鸣叫，`USART1` Debug，J-Link SWD 单步调试
> 完成日期：2026-11-12 ｜ 平台：Windows + PowerShell ｜ 开发前端：CodeBuddy IDE
> 只读输入：`STM32_Buzzer_PWM_Workflow.md` 阶段 5（522–634 行）与附录 B #25–29/#34、`docs/hardware_pins.md`（锁定版）、`docs/env_check.md`（8.3 实测绝对路径、Cortex-Debug 结论）、`docs/build_commands.md`、`docs/debug_guide.md`、阶段 0/2/3/4 归档计划、`Core/Src/main.c`、`Core/Src/buzzer.c`、`build/Debug/RedBull_Buzzer_PWM.elf`
> 硬约束遵守：未使用 SWO/RTT；未复用 PA13/PA14；未做 SWJ remap / 关闭 DBGMCU；Debug 保持 `-O0 -g3`；未升级 J-Link FW；烧录下载经用户当轮授权（F5）。

---

## 0. 结论摘要

| 范围 | 结论 |
| --- | --- |
| 共用①：构建 `0 error`、`.elf/.hex/.bin` 齐全、size 在容量内 | ✅ 已达成（`docs/build_commands.md` 第 5 节） |
| 共用②：一键烧录成功、BOOT0=0、复位从 Flash 启动 | ✅ 已达成（`scripts/flash.ps1` + `flash.jlink` 就位；F5 下载链路实测成功） |
| 共用③：单步暂停时串口停日志、继续后恢复 | ✅ 已达成（用户实测确认） |
| 主选④：`F5` 一次完成「编译 → 连接 J-Link → 下载 → 停在 `main`」，无 error | ✅ 已达成（首次超时经 T0 处置后重试成功，见第 3 节） |
| 主选⑤：`Buzzer_Tick()` 断点稳定命中，`F10` 单步可逐句推进 | ✅ 已达成（用户实测确认） |
| 主选⑥：变量视图四个状态变量随单步变化 | ✅ 已达成（用户实测确认） |
| 主选⑦：SVD 外设寄存器视图可打开并读到寄存器值 | ✅ 已达成（暂停态 `TIM3 @0x40000400` 全寄存器可读，见 2.4） |
| 兜底路径（命令行 GDB） | 产物已就位（`scripts/debug_gdb_cmds.txt`），本阶段未启用：主选可用，按文档 540 行规则无需切换；作为阶段 6 交叉验证手段保留 |
| 硬件动作 | 仅经 F5 下载固件（用户授权）；未改跳线、未飞线、未终止进程、未升级 J-Link FW |

> 判定依据：文档第 621 行「共用 + 主选」或「共用 + 兜底」一组全部满足即通过 → 本阶段以**共用 + 主选**判定**验收通过**。

---

## 1. 交付物清单（本阶段生成/更新）

| 文件 | 职责 | 生成方式 |
| --- | --- | --- |
| `.vscode/tasks.json` | 任务 `CMake Debug Build`：以实测绝对路径调用 `cmake.exe --build build/Debug`，供 `preLaunchTask` 依赖；`group.kind=build` 且 `isDefault=true` | AI 生成 |
| `.vscode/launch.json` | Cortex-Debug J-Link 主选调试配置（逐字段来源见第 2 节） | AI 生成 |
| `scripts/flash.ps1`、`scripts/flash.jlink` | 一键烧录与 J-Link Commander 脚本、下载校验 | 已就位 |
| `scripts/debug_gdb_cmds.txt` | 兜底命令行 GDB 命令序列（保留，本阶段未执行） | 已就位 |
| `docs/build_commands.md` | 构建/清理/重建命令清单与实测证据、size 口径说明 | 已就位 |
| `docs/debug_guide.md` | 连接排查决策树（T0–T6）、下载失败、断点不命中、变量 optimized out 处置 | 已就位 |
| `docs/stage5_report.md` | 本报告 | AI 生成 |

### 1.1 构建证据（`tasks.json` 命令实测）

```text
& '...\cmake-4.5.0-rc3-windows-x86_64\bin\cmake.exe' --build '...\build\Debug'
  → ninja: no work to do.
  → BUILD_EXIT=0
```

完整构建证据（`--clean-first` 全量重建、`0 error`、四件产物、size）见 `docs/build_commands.md` 第 5 节：`text 15764 / data 116 / bss 2100` → Flash **15,880 B**（512 KB 的 3.0%）、RAM **2,216 B**（64 KB 的 3.4%），均在 `STM32F103ZE` 容量内。

---

## 2. 主选调试配置与逐字段来源

### 2.1 字段来源核对表

所有字段均在本机扩展 `package.json` 中核实存在，路径取自阶段 0 实测绝对路径：

| 字段 | 取值 | 来源 |
| --- | --- | --- |
| `type` | `cortex-debug` | 扩展配置类型 |
| `request` | `launch` | schema `launch` 段（1653 行起） |
| `servertype` | `jlink` | schema enum（1658–1666 行） |
| `serverpath` | `C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe` | schema 1798 行；路径见 `env_check.md` 8.3 |
| `device` | `STM32F103ZE` | schema 1778 行 |
| `interface` | `swd` | schema 2729 行（默认值，显式写出） |
| `runToEntryPoint` | `main` | schema 1902 行（默认值，显式写出） |
| `armToolchainPath` | `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin` | schema 1788 行；路径见 `build_commands.md` 53 行 |
| `svdFile` | `...\Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd` | schema 2248 行；路径见 `env_check.md` 8.3（实文件名 `STM32F103xx.svd`，非占位符旧值） |
| `preLaunchTask` | `CMake Debug Build` | 与 `tasks.json` 的 `label` 一致 |
| `executable` | `${workspaceFolder}\build\Debug\RedBull_Buzzer_PWM.elf` | 阶段 4 产物名 |
| `showDevDebugOutput` | `none` | schema 2838 行；排查时可临时改 `raw` |

### 2.2 GDB Server 实际启动参数（扩展自动拼装，实测）

```text
"C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe" -singlerun -nogui -if swd -port 50000 -swoport 50001 -telnetport 50002 -device STM32F103ZE
Launching GDB: "C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-gdb.exe" -q --interpreter=mi2
```

与阶段 0 实证参数（`-if SWD -speed 4000 -device STM32F103ZE`）器件/接口口径一致；端口由扩展默认 50000 系列分配，未冲突；`-speed` 采用扩展默认，实测连接与下载正常。

### 2.3 变量观察结论（主选⑥）

- 断点位置：`Core/Src/buzzer.c` 的 `Buzzer_LoadSegment(next, now)` 调用行（阶段 4 实测稳定命中点）；
- 四个状态变量（段索引、目标频率、实际占空比、剩余时长）在变量视图中可读且随 `F10` 单步变化；
- 未被优化：Debug 实测 `-O0 -g3`（`build.ninja` FLAGS 核对，见 `build_commands.md` 第 4 节），状态变量为 `volatile`（阶段 4 静态自检）。

### 2.4 SVD 外设寄存器视图（主选⑦）

- 暂停态下 `XPERIPHERALS` 成功展开 `TIM3 @0x40000400`，`CNT/PSC/ARR/CCR1–4/DCR/DMAR` 均显示具体值（2026-11-12 用户截图）；
- **`TIM3` 全为 `0x00000000` 属正常**：本工程走路径 A 软件 PWM，使用 **TIM2**（`main.c` 118–127 行：`PSC=72-1`、`Period=100-1` → 10 kHz 更新中断；`buzzer.c` 头注释同步说明）；TIM3 为路径 B 飞线硬件 PWM 的备用通道，未启用；
- 可选加分核对：`TIM2 @0x40000000` 的 `PSC` 应为 `0x47`(71)、`ARR` 应为 `0x63`(99)；`USART1 @0x40013800` 的 `BRR` 在 72 MHz/115200 下应为 `0x271`。

### 2.5 日志与调试同步（共用③）

断点暂停时串口终端停止输出新日志，`F5` 继续后恢复 → halt/恢复链路可用（用户实测确认），佐证调试与日志共用同一执行流、无后台阻塞任务。

---

## 3. 异常与处置记录

### 3.1 首次 F5 报 `Failed to launch jlink GDB Server: Timeout`

| 项 | 记录 |
| --- | --- |
| 现象 | GDB Server 终端停在 `Connecting to J-Link...`，随后 `SERVER CONSOLE DEBUG: onBackendConnect: gdb-server session closed`，日志报 `Failed to launch jlink GDB Server: Timeout` |
| 排查①（T0 进程） | `Get-Process JLink*, JFlash*, GDB*` → 无任何残留进程 |
| 排查②（T0 端口） | `Get-NetTCPConnection -LocalPort 50000,50001,50002,2331` → 全部空闲 |
| 排查③（二进制可用性） | 手工启动 `JLinkGDBServerCL.exe -if swd -port 2331 -device STM32F103ZE -speed 4000`：10 秒内进程存活、正常打印启动设置 → J-Link 二进制与驱动正常 |
| 处置 | **重试 `F5` 一次即成功**（编译 → 连接 → 下载 → 停 `main`） |
| 判定 | 首次启动瞬时超时（USB 枚举 / 安全软件首次扫描 exe），非配置缺陷；无需修改 `launch.json` |
| 登记规则 | 再遇同现象：按 `debug_guide.md` T0 查进程/端口 → 重试一次 → 连续 2 次失败再切兜底，不盲目改配置 |

### 3.2 外设视图 `readMemory failed @ 0x40000400 / 0x40013800`

| 项 | 记录 |
| --- | --- |
| 现象 | `peripheral-viewer: readMemory failed ... Unable to read memory (data-read-memory-bytes "0x40000400" 80)`（`USART1 @0x40013800` 同） |
| 成因 | J-Link GDB Server 在 target **运行态**拒绝读 APB 外设内存；`peripheral-viewer` 自行轮询且未暴露「仅暂停时刷新」开关（已核对扩展 `package.json`，无相关配置项） |
| 处置 | 在**暂停态**打开/刷新外设视图 → `TIM3` 寄存器全部可读，报错消失 |
| 判定 | 行为限制而非配置缺陷；不影响主选⑦验收（附录 B #28 允许「SVD 视图可打开」或「GDB 可读 TIM/RCC 寄存器」） |
| 使用规范 | 外设寄存器观测一律在断点命中（⏸）后进行；运行态的该报错提示可忽略 |

---

## 4. 验收对照（附录 B）

| # | 验收项 | 结论 | 证据 |
| --- | --- | --- | --- |
| 25 | 图形化主选：`F5` 一次完成编译→连接→下载→停在 main | ✅ | 第 2.2、3.1 节 |
| 26 | `Buzzer_Tick()` 断点稳定命中 | ✅ | 用户实测确认 |
| 27 | 单步可见四个状态变量变化 | ✅ | 第 2.3 节 |
| 28 | SVD 外设寄存器视图可打开 | ✅ | 第 2.4 节（暂停态 `TIM3` 可读） |
| 29 | 手工启动 GDB Server 可识别器件并 halt | ✅ | 第 3.1 节排查③（器件识别/启动）+ F5 停在 `main`（halt 实证） |
| 34 | 主选不可用 → 切兜底 | N/A | 主选可用，无需切换；兜底脚本保留 |

---

## 5. 边界与交接

- 本阶段**只改配置**：未修改任何 C 源码、HAL 驱动、链接脚本与 CMake 构建逻辑；`CMakeLists.txt` 未变动；
- 未执行兜底 GDB 序列（`scripts/debug_gdb_cmds.txt`）；阶段 6 可按需用其做「脱离图形化界面」的独立连接交叉验证；
- 未做任何硬件改动（跳线、飞线、蜂鸣器类型均维持阶段 1/4 锁定状态）；
- 交接阶段 6 的关键结论：主选链路可用、`F5` 全链路可复现；外设视图仅在暂停态有效；首次 GDB Server 超时按「重试一次」处理。
