# 工作区根目录文件说明（构建 / 启动 / 调试链路）

> 项目：STM32F103ZET6 红牛开发板，蜂鸣器 PWM 鸣叫（路径 A），USART1 Debug，J-Link SWD
> 生成日期：2026-11-12 ｜ 说明对象：工作区根目录 5 个文件（`CMakeLists.txt`、`RedbullDemo.code-workspace`、`startup_stm32f103xe.s`、`STM32_Buzzer_PWM_Workflow.md`、`STM32F103ZETx_FLASH.ld`）
> 依据：文件实际内容核对 + `STM32_Buzzer_PWM_Workflow.md` 附录 E 产物清单 + 阶段 2/5 归档计划与报告

---

## 0. 一图看懂：5 个文件在链路中的位置

```text
RedbullDemo.code-workspace          ← IDE 工作区入口（打开项目用它）
        │
STM32_Buzzer_PWM_Workflow.md        ← 全项目"施工总图"（904 行工作流母本，只读不改）
        │ 阶段 2 据此生成
CMakeLists.txt                      ← 构建总指挥（决定编什么、怎么编、怎么链）
        │ 引用
        ├──► cmake/gcc-arm-none-eabi.cmake   （交叉工具链声明）
        ├──► startup_stm32f103xe.s           （启动文件：芯片复位后第一段代码）
        └──► STM32F103ZETx_FLASH.ld          （链接脚本：决定代码/数据落在哪个地址）
        │ 产出
        └──► build/Debug/RedBull_Buzzer_PWM.elf/.hex/.bin/.map
                 │ 阶段 5 消费
                 └──► .vscode/launch.json (F5) / scripts/flash.ps1（烧录 + 单步调试）
```

---

## 1. `CMakeLists.txt` — 顶层构建配置（阶段 2 生成）

**作用**：整个工程的"构建总指挥"，AI 按工作流阶段 2 提示词生成。CMake 读它来决定编译哪些源文件、用什么编译选项、链接哪个脚本、生成什么产物。

**关键内容（与本项目验收口径直接挂钩）**：

| 行号 | 内容 | 对本项目意味着什么 |
| --- | --- | --- |
| 11–14 | 工具链文件 `cmake/gcc-arm-none-eabi.cmake` | 声明使用 `arm-none-eabi-gcc` 交叉编译；路径取 `docs/env_check.md` 实测值 |
| 22–23 | Debug 强制 `-O0 -g3` | 单步调试断点可命中、变量不 optimized out 的**根本保证**（附录 B #26/#27；禁止改 `-Os`） |
| 30–39 | `DEBUG_LOG_LEVEL`（Debug=4，Release=2） | 编译期日志等级开关，可用 `-DDEBUG_LOG_LEVEL=N` 覆盖 |
| 41–48 | `APP_SOURCES`：`main.c / debug_uart.c / buzzer.c / stm32f1xx_it.c / system_stm32f1xx.c / startup_stm32f103xe.s` | 应用层 5 个 C 文件 + 启动文件；阶段 4 定点增补 `buzzer.c` 就是加在这里 |
| 51–66 | `HAL_SOURCES`：15 个官方 HAL 驱动源 | 最小必需集合（依据 `docs/stage2_hal_source.md`），少一个就报 `undefined reference` |
| 79–83 | 编译定义 `STM32F103xE / USE_HAL_DRIVER / DEBUG_LOG_LEVEL` | HAL 库按 F103 大容量型号展开；`BEEP=PB2` 等 GPIO 配置依赖此宏 |
| 92–98 | 链接脚本 `STM32F103ZETx_FLASH.ld` + 生成 `.map` | 指定内存布局；`.map` 文件（504,877 B）用于核对变量地址 |
| 100–105 | POST_BUILD：`objcopy` 生成 `.hex/.bin` + `arm-none-eabi-size` 报告 | 每次构建自动产出四件套（`.elf/.hex/.bin/.map`）并打印 size——这就是附录 B #10–12 的证据来源 |

**改它的后果**：改 `-O0 -g3` → 断点失效；删 HAL 源 → 链接报错；换链接脚本 → Flash 布局错乱无法启动。**阶段 3/4/5 均未改动本文件（仅阶段 4 经 `APP_SOURCES` 增补 `buzzer.c`）。**

---

## 2. `RedbullDemo.code-workspace` — IDE 工作区文件

**作用**：CodeBuddy/VS Code 的**多根工作区描述文件**，双击/打开它即以正确的上下文加载本项目。

**实际内容**（极简，仅 9 行）：声明一个名为 `20261110233826` 的文件夹指向当前目录，`settings` 为空。

- 目前**无项目级配置**——调试与构建配置不在这里，而在 `.vscode/tasks.json`（构建任务）与 `.vscode/launch.json`（Cortex-Debug 主选调试配置，阶段 5 生成）。
- 定位：入口/容器文件，本身不参与编译烧录；打开工作区是为了让 IDE 正确索引 `Core/`、`Drivers/`、`docs/`、`scripts/` 等目录，使 `F5`、内置终端、`@` 引用都能工作。
- 可保留现状：除非需要加排除目录或编辑器偏好，不需要动它。

---

## 3. `startup_stm32f103xe.s` — 启动文件（ST 官方，未修改）

**作用**：芯片**复位后执行的第一段代码**（汇编），是固件的"起跑线"。链接脚本把它的向量表放在 Flash 最开头（`0x08000000`）。

**做四件事**：

1. **设初始栈指针（SP）**：取链接脚本的 `_estack = 0x2000FFFF`（RAM 顶端）；
2. **设初始 PC = `Reset_Handler`**：上电/复位后从这里开始跑；
3. **提供中断向量表 `g_pfnVectors`**：把每个异常/中断入口（含本工程用到的 **TIM2 更新中断**、USART1 中断位）指到 `stm32f1xx_it.c` 里的处理函数——蜂鸣器 10 kHz 软件 PWM 靠它进中断；
4. **搬运行时数据**：把 `.data` 初值从 Flash 复制到 RAM、把 `.bss` 清零，然后调 `SystemInit()`（时钟配置）→ 跳进 C 库 → 进入 `main()`。

**与本项目的实测关联**：阶段 3 里 J-Link halt 读到 PC=`0x08004FE6`、复位向量 `0x08005c69`，指的就是这段代码被链接到主 Flash 后的地址——这也正是 `BOOT0=0`（从 User Flash 启动）才能单步调试的原因。

**禁止改动**：它是 ST 官方模板（Atollic/GCC 语法），与 `STM32F103xE` 宏和链接脚本配套；改向量表或 `_estack` 会导致无法启动或栈溢出。

---

## 4. `STM32_Buzzer_PWM_Workflow.md` — 工作流方案母本（只读，全项目"施工总图"）

**作用**：904 行的分阶段 AI 生成工作流方案，**整个项目的最高层依据**。它不是固件的一部分，不参与编译。

**地位与使用方式**：

- 由两份已完成计划产出：`53cdae2a`（初版创作）→ `108500a3`（迁移到 CodeBuddy 版），阶段 0–6 全部按它的六要素（输入/提示词/产出物/验证点/验收标准/失败回退）执行；
- **核心内容**：阶段 0 环境校验 → 阶段 1 原理图解析与引脚映射表模板 → 阶段 2 工程骨架 → 阶段 3 串口 Debug → 阶段 4 PWM 业务 → 阶段 5 构建与 J-Link 调试双路径 → 阶段 6 端到端验证闭环；附录 A–G（提示词模板、验收勾选表 #1–34、故障排查表、回退话术、产物清单、硬件速查卡、术语）；
- **已交付的"验收锚点"**：阶段 6 的验证点表（689–700 行）、五条判定（704–710 行）、回退矩阵（712–720 行）——`docs/acceptance_checklist.md` 与 `docs/troubleshooting.md` 就是对这三段的落地展开；
- **硬纪律**：只读引用（`@` 引用），任何阶段不得改写它；占位符（`{MCU}`、`{BUZZER_PIN}` 等）一律以 `docs/hardware_pins.md` 锁定版回填为准。

---

## 5. `STM32F103ZETx_FLASH.ld` — 链接脚本（官方模板，未修改）

**作用**：告诉链接器**固件的每个部分放到哪个地址**。烧录、启动、调试能否成功，首先取决于它。

**关键事实（与硬件真相源对齐）**：

| 条目 | 值 | 含义 |
| --- | --- | --- |
| `ENTRY(Reset_Handler)` | — | 入口 = 启动文件的复位处理函数 |
| `_estack` | `0x2000FFFF` | 主栈顶 = RAM 末尾 |
| `FLASH` | `ORIGIN 0x08000000, LENGTH 512K` | 与 STM32F103ZE 实际 512 KB Flash 一致；`.bin` 15,880 B 只占 3.0% |
| `RAM` | `ORIGIN 0x20000000, LENGTH 64K` | 与 64 KB SRAM 一致；data+bss 2,216 B 占 3.4% |
| `.isr_vector` | `KEEP(*(.isr_vector))` 放 Flash 最前 | 向量表不被 `--gc-sections` 裁掉——中断（TIM2 10 kHz PWM）能进的前提 |
| 堆/栈下限 | heap 0x200 / stack 0x400 | 链接期保证 RAM 放得下 |

**下游依赖它的地方**：`CMakeLists.txt` 92–98 行（`-T` 传给链接器 + `LINK_DEPENDS` 改动即触发重链）；`docs/debug_guide.md` 第 2 节烧录失败排查时核对 `ORIGIN/LENGTH` 与器件一致；GDB 判断 PC 是否在 `0x080xxxxx`（主 Flash）也以此布局为基准。

**禁止改动**：`LOAD segment with RWX permissions` 链接警告即来自此官方模板的段属性，属既有非 error 警告，勿"修复"。

---

## 6. 五个文件的协作关系总结

| 文件 | 生成来源 | 能不能改 | 改动影响面 |
| --- | --- | --- | --- |
| `CMakeLists.txt` | 阶段 2 AI 生成（阶段 4 增补一行） | ⚠️ 谨慎（`-O0 -g3` 与 HAL 源列表是红线） | 断点/变量观察、链接成败、日志等级 |
| `RedbullDemo.code-workspace` | IDE 生成 | ✅ 一般无需改 | 仅 IDE 上下文，不参与固件 |
| `startup_stm32f103xe.s` | ST 官方（阶段 2 引入） | ❌ 禁止 | 复位流程、中断向量、栈初始化 |
| `STM32_Buzzer_PWM_Workflow.md` | 两份已完成计划产出 | ❌ 只读（`@` 引用） | 全项目依据链，改写=重写历史 |
| `STM32F103ZETx_FLASH.ld` | ST/Ac6 官方模板（阶段 2 引入） | ❌ 禁止 | Flash/RAM 布局、烧录与启动成败 |

**一句话**：`.md` 是施工总图，`.code-workspace` 是工地大门，`CMakeLists.txt` 是施工调度，`.s` 和 `.ld` 分别是"开机第一步"和"东西摆在哪"——四者合力产出 `.elf/.hex/.bin`，再交给 `F5` / `flash.ps1` 烧进板子。
