# RedBullAI

STM32F103ZET6 红牛开发板蜂鸣器 PWM 工程：**蜂鸣器按节奏鸣叫 + USART1 串口 Debug 输出 + J-Link SWD 单步调试**。

整个工程（源码、构建配置、调试配置、文档）全部由 AI 生成，作者零手写代码。创建过程的复盘与可复用套路见 [`docs/全AI零代码创建工程复盘与后续开发指引.md`](docs/全AI零代码创建工程复盘与后续开发指引.md)。

## 项目信息

| 项 | 内容 |
| --- | --- |
| 目标芯片 | STM32F103ZET6，LQFP144，Cortex-M3，512 KB Flash / 64 KB RAM |
| 时钟 | HSE 8 MHz → PLL → SYSCLK 72 MHz |
| 蜂鸣器 | PB2（pin48），有源、高电平有效，路径 A 软件 PWM（TIM2 更新中断 10 kHz，PSC=71 / ARR=99） |
| 鸣叫序列 | 三短鸣 → 停 0.6 s → 一长鸣 0.8 s → 静音 2 s，循环周期 4150 ms |
| Debug 串口 | USART1，PA9/PA10，115200-8N1，`printf` 经 `_write` 重定向 |
| 固件基础 | ST 官方 STM32CubeF1 HAL + CMSIS（V1.8.7），不使用 StdPeriph |
| 构建 | CMake + Ninja + arm-none-eabi-gcc，Debug 固定 `-O0 -g3` |
| 调试 | J-Link SWD；Cortex-Debug 图形化主选（F5），命令行 GDB 兜底 |
| 平台 | Windows + PowerShell |

当前固件占用：Flash 15,880 B / RAM 2,216 B（阶段 4 实测，容量内）。

## 当前状态

阶段 0–5 已完成并通过验收：工程骨架、串口日志、PWM 鸣叫业务、F5 全链路（编译→连接→下载→停在 `main`）、断点观察四个状态变量均已实测闭环，蜂鸣器鸣叫节奏经用户听感确认。

阶段 6 的端到端 10 步清单与 `docs/delivery_report.md` 证据回填**待执行**（烧录步骤需当轮显式授权），最终验收尚未判定——如实标注，不虚报通过。

## 目录结构

```text
.
├── CMakeLists.txt / STM32F103ZETx_FLASH.ld / startup_stm32f103xe.s
├── cmake/            交叉工具链配置
├── Core/Inc, Core/Src/   main、GPIO/时钟/USART 初始化、串口日志、蜂鸣器业务与中断
├── Drivers/          CMSIS 与 STM32F1xx HAL 官方最小源集合
├── build/Debug/      构建产物 .elf/.hex/.bin/.map
├── scripts/          env_check.ps1、flash.ps1/flash.jlink、debug_gdb_cmds.txt
├── docs/             阶段报告、接线、时钟树、鸣叫序列、构建/调试、验收与排查文档
├── .codebuddy/plans/ 阶段 0–6 的归档计划
├── STM32_Buzzer_PWM_Workflow.md   全 AI 生成工作流剧本（阶段门 + 六要素）
├── 红牛开发板电路图/用户手册.pdf   只读硬件资料
└── RedbullDemo.code-workspace
```

## 快速开始

```powershell
# 1) 环境校验（工具链版本、J-Link、SVD、绝对路径；只读幂等）
powershell -File scripts\env_check.ps1

# 2) 配置 + 构建（在工程根目录执行；工具路径以 docs/env_check.md 实测为准）
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug

# 3) 烧录（需 BOOT0=0 且用户当轮授权）
powershell -File scripts\flash.ps1

# 4) 调试
#   图形化：CodeBuddy + Cortex-Debug，按 F5（编译→下载→停 main）
#   兜底：scripts/debug_gdb_cmds.txt（命令行 GDB）
```

完整命令、PATH 补齐、size 口径与排错决策树见 [`docs/build_commands.md`](docs/build_commands.md) 与 [`docs/debug_guide.md`](docs/debug_guide.md)。

## 文档索引

| 类别 | 文档 |
| --- | --- |
| 硬件真相 | `docs/hardware_pins.md`（锁定版引脚映射表，唯一硬件真相源） |
| 环境与来源 | `docs/env_check.md`、`docs/stage2_hal_source.md` |
| 实现与时钟 | `docs/clock_tree.md`、`docs/init_selfcheck.md`、`docs/buzzer_sequence.md` |
| 接线 | `docs/uart_wiring.md`、`docs/pwm_path_wiring.md` |
| 构建与调试 | `docs/build_commands.md`、`docs/debug_guide.md` |
| 阶段报告 | `docs/stage2_build_report.md` ~ `docs/stage5_report.md` |
| 验收与交付 | `docs/acceptance_checklist.md`、`docs/troubleshooting.md`、`docs/delivery_report.md` |
| 复盘指引 | `docs/全AI零代码创建工程复盘与后续开发指引.md` |

## 硬约束（不要破）

- 用户零手写代码，所有源码与配置由 AI 生成。
- 不复用 PA13/PA14，不做 SWJ remap，不关 DBGMCU，Debug 固定 `-O0 -g3`，禁用 SWO/RTT。
- 有源蜂鸣器不做变频变调逻辑（频率字段只代表通断节奏）。
- 引脚、HAL API、工具路径、J-Link 参数一律实测确认，不凭记忆编造。
- 烧录/下载每次需当轮显式授权；验收只写可观测证据，未测项标"待人工确认"。
