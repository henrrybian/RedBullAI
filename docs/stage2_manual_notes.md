# 阶段 2：用户手册解析补充结论（跳线与 BOOT0 风险）

> 项目：STM32F103ZET6 红牛开发板 ｜ 日期：2026-11-12 ｜ 平台：Windows + PowerShell
> 只读输入：`红牛开发板用户手册(RedBull Manual).pdf`（779,709 字节）；对照 `@docs/hardware_pins.md`（锁定版）、`@docs/env_check.md`、`@.codebuddy/plans/阶段0-资料与环境准备-环境校验与就绪判定_26b1905e.md`、`@.codebuddy/plans/阶段1-原理图解析与PB2寄存器实测_aff9afe7.md`。
> **边界声明**：本文只补充手册文字层给出的风险提示与仍待人工确认项；**不改写 `@docs/hardware_pins.md` 的任何锁定结论**，不把手册推断覆盖阶段 1 的实测结论，不重复解析原理图。

---

## 1. 解析方式与证据边界（实测，非臆造）

| 项 | 实测结果 |
| --- | --- |
| PDF 头 / 大小 | `%PDF-1.4`，`779,709` 字节；`Get-Item -LiteralPath` 确认可读 |
| 结构 | `/Type /Page` 9 个（9 页）、`FlateDecode` 24 处、`/Subtype /Image` 4 个、`/Type /Font` 20 个 |
| 工具探测 | `python.exe` / `python3.exe` 仅为 `C:\Users\HenrryBian\AppData\Local\Microsoft\WindowsApps\` 的 Microsoft Store 别名，实测报错 `Python was not found`；`pdftotext` / `qpdf` / `mutool` / `magick` / `tesseract` 均无 → **[skill:pdf] 的 Python 库与命令行工具路径在本机不可用**（与 `@docs/hardware_pins.md` 第 18 行「本机无 Python / Node / poppler」一致） |
| 回退路径 | 沿用阶段 1 已实证的 PowerShell + .NET `DeflateStream` 解压 24 个 `FlateDecode` 内容流，抽取 `(…)Tj` / `[…]TJ` 明文，重建 683 行纯文本（15,924 字节） |
| 文字层状态 | **文字层存在**（`BT` 105 处、`Tj|TJ` 1,141 处），故未触发 [skill:pdf] 的页面渲染 / OCR 回退路径；中文段落为 CID 编码 `<…>Tj`，无 ToUnicode 解码则不可读，**本文只对可读的英文/数字明文下结论，中文段落一律不推断** |
| 页面证据口径 | 因本机无 PDF 库无法可靠输出物理页码，本文以 **章节号 + 内容流序号（STREAM n）** 作为可复现的页面证据；抽取产物位于 `$env:TEMP\stage2_manual_text.txt`，可由本文第 1 节命令复现 |

复现命令要点：

```powershell
# 1) 按字节取 PDF，Latin1 映射后定位 (?<!end)stream\r?\n … endstream
# 2) 逐流尝试 .NET DeflateStream（跳过 zlib 2 字节头），StreamReader 还原内容流
# 3) 仅保留含 Tj/TJ 的行，提取 \(…\) 内字符串，按 STREAM 分节写出 UTF-8 文本
```

关键词实测命中（重建纯文本）：`JP6=2`、`JP8=1`、`JP9=1`、`BOOT0=2`、`BOOT1=3`、`JTAG=4`、`SWD=4`、`RS232=3`、`JLINK=5`、`STM32=9`；**`JP5=0`、`BEEP=0`、`Vref=0`、`Version=0`、`RedBull=0`**。

---

## 2. 手册补充结论（章节 / 内容流证据）

| # | 章节（内容流） | 手册文字层原文摘录 | 与锁定版对照 | 结论 / 处置 |
| --- | --- | --- | --- | --- |
| M1 | 第 1 章 概览（STREAM 9） | `ARM Cortex-M3`；`CPU STM32F103ZET6;(LQFP144 , 512K flash 64KRAM)` | `@docs/hardware_pins.md` 39–41 行把封装名 / 内核 / Flash / SRAM 列为「待人工确认」；官方 CMSIS 模板为 512K FLASH / 64K RAM | **补充证据，不改写锁定表**：手册与官方链接脚本容量一致，但按硬约束仍需人工目视芯片丝印 / 查 ST 数据手册后才可回填 D1/D2 |
| M2 | 2.1 Power supply（STREAM 13） | `2.1 Power supply`；`5V DC J3`；`JP4 1-2 5V` / `JP4 2-3 USB 5V`；`JLINK ARM J5`、`JLINK COMMANDER power on` | 锁定版 `{POWER_JP}`=`JP4 Power` 3 针 pin2 接 5V；`@docs/env_check.md` 硬件禁忌 #1/#4 | **供电风险补充**：JP4 决定 5V 来源；J-Link 由 `J5` 侧取电。本阶段不改跳线、不带电插拔，Vref/共地仍待人工实测 |
| M3 | 2.2 Boot option（STREAM 13） | `Embedded user Flash` / `System memory with boot loader for ISP` / `Embedded SRAM for debugging`；`BOOT1`、`JP8`、`BOOT0`、`JP9`；`ANY`、`1-2`、`2-3 or open`、`2-3 (Default setting)`、`User Flash`、`System Memory`、`Embedded SRAM` | **锁定版：`JP8 BOOT0`（3 针：1=3V3 / 2→R24 10K→BOOT0 pin138 / 3=GND），`JP9 BOOT1`（pin2 经 R25 10K 与 PB2 共网络）** | ⚠️ **重大差异**：手册文字层把 `JP8` 与 `BOOT1`、`JP9` 与 `BOOT0` 并列（内容流 Tm 坐标核对：`(BOOT1)Tj` → `(JP8)Tj`；`[( BOOT0)]TJ` → `(JP9)Tj`），与原理图锁定版编号**相反**。可能是表格列序、板卡版本或丝印差异；**不改写锁定版、不据此改跳线**，列为阶段 5 前最高优先级人工目视 + 万用表确认项 |
| M4 | 2.2 Boot option（同上） | 启动模式表含 `2-3 (Default setting)`，与 `System Memory` / `Embedded SRAM` 行相邻 | `@docs/hardware_pins.md` 第 4 节 #3 要求 `BOOT0=0`（主 Flash 启动）；附录 B.2 第 282 行实测 `pc = 0x1FFFF3B6`（系统存储器 / bootloader 区） | ⚠️ **BOOT0 风险互相印证**：手册标注了默认跳线档位，且 halt 时 PC 落在 bootloader 区，提示当前 `JP8 BOOT0` 可能非 0。阶段 2 **只做识别与 halt 验证，不烧录**；从 Flash 单步前必须按锁定版 #3 人工复核 `BOOT0=0` |
| M5 | 2.3 Clock source（STREAM 14） | `Y1 32.768kHZ RTC`、`Y2 8MHZ`、`8M RC`；`Jumper JP4` 供电选择 | 锁定版 `{HSE_MHZ}=8`（`Y2 8MHz`）、`{LSE_KHZ}=32.768`（`Y1`） | ✅ 与锁定版一致，互相印证；不改变工程 HSE 8 MHz / SYSCLK 72 MHz 配置 |
| M6 | 3.4 RS232 / CN1（STREAM 18） | `3.4 RS232`；`CON1`、`CON2`；`RS232 connector CON1 CON2 (front view)`；`JP12` / `Default setting: not fitted`；CN1 引脚表 `1 AIN0 2 AIN1 3 AIN2 4 GND 5 PWM0 6 PWM1 7 GND 8 DAC0 9 DAC1 10 GND` | 锁定版 `{DEBUG_LEVEL}` = RS232 电平（`U5 ST3232`），`CON1`/`CON2` 不可直连 USB-TTL；锁定版 P3 渲染核对 CN1 = pin4 PB0/PWM0、pin5 PB1/PWM1、pin8 PA4/DAC0、pin9 PA5/DAC1 | ⚠️ 两处差异：①新增手册跳线 `JP12`（锁定版未列，默认不安装）；②CN1 引脚编号与锁定版不同 → 归入「板卡版本一致性」复核项 #9。**不改写锁定表**；`@docs/env_check.md` 341 行已人工确认 `CON1` 配 RS232-USB 线，串口实际接线方式仍按阶段 2 计划 93 行保留为遗留项 |
| M7 | 3.5 JTAG / 3.7 JTAG·SWD（STREAM 19 / 20） | `3.5 JTAG J5`；`3.7 JTAG SWD`；`JP6 JTAG` / `JP6 SWD`；`TMS SWDIO`、`TCK SWDCLK`；右半表 `11 RTCK 12 GND`、`13 PB3 14 GND`、`15 RESET# 16 GND`、`17 DBGRQ 18 GND`、`19 DBGACK 20 GND`；左半表 `1 NTRST PB4`、`2 TDI PA15`、`3 TMS PA13`、`4 TCK PA14`、`5 TDO PB3` | 锁定版 `{SWDIO_PIN}=PA13`、`{SWCLK_PIN}=PA14`、`{JTAG_SEL_JP}=JP6 JTAG_SEL` 10 针、`{DEBUG_CONN}=J5 JTAG` 20 针（pin1 Vref / pin2 Vsupply / pin3 nTRST / pin5 TDI / pin7 TMS / pin9 TCK / pin13 TDO / pin15 nSRST…偶数脚 GND） | ✅ 信号映射一致（TMS→PA13、TCK→PA14、偶数脚 GND）；⚠️ J5 左半表物理针序与锁定版不同（表格含义或板卡版本差异，不改写）。`@docs/env_check.md` 357 行记录用户确认 `JP6` 当前为 **JTAG 模式**，与 SWD 目标冲突 → 阶段 2 J-Link 只做识别/halt，切换跳线须先断电且由人工执行 |
| M8 | 3.6 USB2.0（STREAM 20 前） | `3.6 USB2.0` | 与阶段 2 无关 | 仅记录章节存在，不纳入本阶段配置 |
| M9 | 全文检索 | `JP5=0`、`BEEP=0`、`Version=0`、`RedBull=0` | 锁定版 `{BEEP_JP}=JP5 BEEP`；蜂鸣器有源 / 高电平有效已由 11-12 GDB 实测闭环 | 手册文字层**未给出** `JP5` / 蜂鸣器信息：锁定版结论不受影响，`JP5` 是否短接仍待人工目视；板卡版本字符串未出现，复核项 #9 仍「待人工确认」 |

---

## 3. 纳入阶段 2 报告的风险与遗留清单

| # | 风险/遗留 | 来源 | 阶段 2 处置 |
| --- | --- | --- | --- |
| R1 | `JP8`/`JP9` 与 `BOOT0`/`BOOT1` 的编号对应在手册与原理图间相反 | M3 | 不改写锁定版；人工目视 + 万用表确认；阶段 5 前必须闭环 |
| R2 | `pc = 0x1FFFF3B6` 提示 `JP8 BOOT0` 当前可能非 0；手册标有 `(Default setting)` 跳线档位 | M4 + `@docs/hardware_pins.md` 282、323 行 | 纳入 `docs/stage2_build_report.md` 遗留项；不烧录、不改跳线 |
| R3 | `JP6` 当前为 JTAG 模式，与 SWD 目标冲突 | M7 + `@docs/env_check.md` 357、382 行 | J-Link 验证只用已实测生效的 `-device STM32F103ZE -if SWD -speed 4000 -port 2331`；不带电改跳线 |
| R4 | J-Link 句柄可能被 `JFlash`/`JLinkGUIServer` 占用；V8 兼容/克隆类 FW 升级暂缓 | `@docs/env_check.md` 385、408、432 行 | 只记录风险，不强杀进程、不升级 FW、不改驱动 |
| R5 | `JP12` 默认未安装、CN1 针序与锁定版不同、板卡版本无法从手册确认 | M6 / M9 | 归入板卡版本复核项 #9；Debug 串口实际接线方式仍待用户回传 |
| R6 | `CON1` DB9 为 RS232 电平，USB-TTL 直连会损坏 | M6 + `@docs/hardware_pins.md` 59、146 行 | 工程只初始化 USART1 引脚，不产生任何电平转换假设 |
| R7 | SWD 引脚 PA13/PA14 全程不得复用为 GPIO，禁止 remap/DBGMCU 关闭 | 工作流 316 行、`@docs/env_check.md` 170 行 | 阶段 2 `MX_GPIO_Init` 只配置 USART1 与路径 A 所需 GPIO，不触碰 PA13/PA14 |

---

## 4. 边界自检

- 未修改 PDF、未修改 `@docs/hardware_pins.md`、`@docs/env_check.md`、`STM32_Buzzer_PWM_Workflow.md`、`scripts/env_check.ps1` 与 `RedbullDemo.code-workspace`。
- 未产生 `.c/.h/.s/.ld`、`CMakeLists.txt`、`.ioc`、调试配置或固件（本 todo 只产出本说明文档）。
- 未接板、未上电、未烧录、未改跳线、未改驱动、未升级 J-Link FW、未终止任何 J-Link 进程。
- 未使用 StdPeriph 替代 Cube HAL；手册结论仅作风险提示，不作为引脚真相源。
