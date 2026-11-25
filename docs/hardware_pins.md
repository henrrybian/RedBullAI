# 阶段 1：硬件引脚映射表（唯一硬件真相源 · 锁定版）

> 项目：STM32F103ZET6 红牛开发板，蜂鸣器 PWM 鸣叫，`USART1` Debug 输出，J-Link SWD 调试。
> 生成日期：2026-11-11 ｜ 平台：Windows + PowerShell ｜ 开发前端：CodeBuddy IDE
> 依据：`STM32_Buzzer_PWM_Workflow.md` 阶段 1（175–282 行）、`docs/env_check.md`、阶段 0 归档计划、`红牛开发板电路图(RedBull Schematic).pdf`。
> **本文一旦落盘即锁定**，后续阶段通过 `@` 引用，不得重复猜测引脚。

---

## 0. 交付边界与真相源

**本次只完成阶段 1**，产出 `docs/hardware_pins.md`，并对 `STM32_Buzzer_PWM_Workflow.md` 的 0.1 表做两行定点回填（仅 `{SVD}` / `{CB_EXT_AVAILABLE}`）。

**没有生成、也不会生成本阶段生成**：任何 `.c/.h/.s/.ld`、`CMakeLists.txt`、`.ioc`、`.elf/.hex/.bin`、`launch.json` 或调试配置；**未编译工程、未烧录固件**。PB2 实测走 J-Link GDB Server + `arm-none-eabi-gdb` 直接读写内存映射寄存器，替代工作流 257/281 行的「最小 GPIO 测试程序」路径。

**PDF 逻辑页序（实测，非臆造）**：页树 `/Kids[59 0 R 1 0 R 4 0 R]`，故 **P1 = obj 59（MCU 主图 / 蜂鸣器 / 跳线 / 调试口）**、**P2 = obj 1（外扩 NOR/SRAM/NAND 存储器）**、**P3 = obj 4（RS232 / RS485 / CAN / CN1 扩展口）**。此顺序与工作流 1.1 表的页码标注一致。

**证据获取方式（双向核对）**：本机无 Python / Node / poppler，故用 PowerShell + .NET `DeflateStream` 解压 13 个 `/FlateDecode` 内容流，按 `BT…ET` 文本块抽取 `(…)Tj` / `[…]TJ` 明文（P1 647 块 / P2 217 块 / P3 115 块，`hexTj` 仅 1 处）；再用 WinRT `Windows.Data.Pdf.PdfDocument` 把 3 页渲染为 PNG 并放大裁剪做视觉核对。**文字层未缺失，故未触发 OCR / 万用表回退。** 渲染 PNG 与放大裁剪图均为临时核对产物，已清理；可按上述方法从工作区 PDF 复现，本阶段交付物只含 `docs/hardware_pins.md`（另加 0.1 表两行回填）。

> **视觉核对补齐了文字层的遗漏**：P1 图中央标注 `STM32F103ZET6`，而文字层因字符分片只抽到 `U1STM32F103`。完整型号以渲染图为准，证据页码 P1。

**置信度定义**：

| 等级 | 含义 |
| --- | --- |
| 高 | 图纸文字明确写出，或渲染图清晰可读；可逐字引用 |
| 中高 | 由电路拓扑推断且经渲染图核对，但需实测确认 |
| 中 | 由电路拓扑或器件常识推断，无直接图纸文字证据 |
| 低 | 仅间接线索 |
| 待人工确认 | 图纸未写出，按硬约束禁止臆造，须按第 3 节方法实测 |

---

## 1. 硬件引脚映射表（字段名固定，供后续提示词引用）

| 字段名 | 值 | 证据页码 | 图纸事实 / 电路推断 | 置信度 |
| --- | --- | --- | --- | --- |
| `{MCU}` | `STM32F103ZET6` | P1（图中央标题；文字层 `U1STM32F103`） | 图纸事实（视觉） | 高 |
| `{MCU_PACKAGE}` | 144 脚；封装名（LQFP144）**待人工确认** | P1 `Vdd_3 144`、`Vss_3 143`、引脚号 1–144 | 图纸事实（144 脚）/ 推断（封装名） | 高 / 待人工确认 |
| `{MCU_CORE}` | **待人工确认**（图纸未写内核） | — | 图纸未标注 | 待人工确认 |
| `{MCU_FLASH}` / `{MCU_SRAM}` | **待人工确认**（图纸未写 MCU 内部容量；P2 为板载外扩 `M29W128FH` / `IS61LV51216` / `NAND512`，不是 MCU 内部资源） | P2 | 图纸未标注 MCU 内部容量 | 待人工确认 |
| `{HSE_MHZ}` | `8` MHz（`Y2 8MHz`） | P1 `Y28MHz` | 图纸事实 | 高 |
| `{HSE_PIN}` | `OSC_IN`(23) / `OSC_OUT`(24)，`C21`/`C22` 20 pF | P1 `OSC_IN23`、`OSC_OUT24`、`C2120pF`、`C2220pF` | 图纸事实 | 高 |
| `{LSE_KHZ}` | `32.768` kHz（`Y1 32.768KHz`） | P1 `Y132.768KHz` | 图纸事实 | 高 |
| `{LSE_PIN}` | `PC14-OSC32_IN`(8) / `PC15-OSC32_OUT`(9)，`C19`/`C20` 10 pF | P1 `PC14-OSC32_IN8`、`PC15-OSC32_OUT9`、`C1910pF`、`C2010pF` | 图纸事实 | 高 |
| `{BUZZER_NET}` | `BEEP/BOOT1`（网络标签），驱动侧另标 `BEEP` | P1 `BEEP/BOOT1`、`BEEP`、`JP5BEEP`、`DZ1BEEP` | 图纸事实 | 高 |
| `{BUZZER_PORT}` / `{BUZZER_PIN}` | `GPIOB` / `GPIO_PIN_2`（PB2） | P1 `PB2/BOOT148`、`R2510K`、`PB2` | 图纸事实 | 高 |
| `{BUZZER_PHYSICAL_PIN}` | LQFP144 **pin 48** | P1 `PB2/BOOT148` | 图纸事实 | 高 |
| `{BUZZER_DRIVER}` | `BEEP` → `JP5 BEEP` 使能跳线 → `R39 1K` → `Q1`（图纸仅标 `NPN`，**型号待人工确认**）基极；Q1 发射极接地，集电极接负载；`DZ1` 与 `5V` 同区域出现 | P1 `JP5BEEP`、`R391K`、`Q1NPN`、`DZ1BEEP`、`5V`；渲染图可见拓扑 | 图纸事实（元件/阻值/供电标签）；拓扑由视觉核对 | 高（元件）/ 中高（拓扑） |
| `{BUZZER_ACTIVE_LEVEL}` | **高电平有效（已实测确认）**：PB2=1 蜂鸣器持续鸣叫，PB2=0 静音 | P1 NPN 开关拓扑 + 11-12 GDB 寄存器实测（附录 B.2）：`ODR` 置 1 = `0x14`、置 0 = `0x10`；用户回报「15 秒持续鸣叫」 | 图纸推断 + **实测确认** | 高 |
| `{BUZZER_TYPE}` | **有源蜂鸣器（已实测确认）**；本体型号仍「待人工确认」（照片上被贴纸盖住） | 11-12 PB2 恒高 15 秒持续鸣叫（附录 B.2）；图纸 P1 仅有 `BEEP` 符号，无型号 | **实测确认**（类型）/ 待人工确认（型号） | 高 / 待人工确认 |
| `{BUZZER_TIM}` / `{BUZZER_TIM_CH}` | **PB2 无任何 TIMx_CHy 复用**；飞线备选 `TIM3` / `TIM3_CH3`（PB0） | P1 `PB2/BOOT148`（无 TIM 字段）对比 `PB0/ADC12_IN8/TIM3_CH3/TIM8_CH2N46` | 图纸事实 | 高 |
| `{PWM_FALLBACK_PIN}` | `CN1 CON10`：**PWM0 = PB0 / TIM3_CH3（pin 46，CN1 pin4）**、**PWM1 = PB1 / TIM3_CH4（pin 47，CN1 pin5）** | P1 `PB0/.../TIM3_CH3/...46`、`PB1/.../TIM3_CH4/...47`；P3 `CN1CON10`、`PWM0`、`PWM1`、`PC0PC1PC2PB0PB1PA4PA5` 与渲染图 pin1–10 | 图纸事实（文字 + 视觉） | 高 |
| `{PWM_OTHER_TIM_PINS}` | P1 引脚表另见 `PA6/TIM3_CH1`(42)、`PA7/TIM3_CH2`(43)、`PB6/TIM4_CH1`(136)、`PB7/TIM4_CH2`(137)、`PB8/TIM4_CH3`(139)、`PB9/TIM4_CH4`(140)；**是否已引到扩展口并可用作硬件 PWM：待人工确认** | P1 引脚表 | 图纸事实（复用功能）/ 推断（引出到扩展口） | 高 / 待人工确认 |
| `{DEBUG_USART}` | `USART1`（另有 `USART2`、`USART3`） | P1 `PA9/USART1_TX/...101`、`PA10/USART1_RX/...102`；P3 `U1TX/U1RX/U2TX/U2RX` | 图纸事实 | 高 |
| `{DEBUG_TX_PIN}` / `{DEBUG_RX_PIN}` | `PA9`(pin 101) / `PA10`(pin 102)；网络名 `U1TX` / `U1RX` | P1 引脚表；P3 `RS232_RX1RS232_TX1PA9PA10` | 图纸事实 | 高 |
| `{DEBUG_UART_LEVEL_CHIP}` | `U5 ST3232` + `C39`~`C42` 100 nF | P3 `U5ST3232`、`C39100nF`~`C42100nF` 与渲染图 | 图纸事实 | 高 |
| `{DEBUG_UART_CONN}` | `CON1 UART1`（DB9，9 针）；`CON2 UART2`（DB9，9 针） | P3 `CON1UART1`、`CON2UART2` 与渲染图 1–9 针 | 图纸事实 | 高 |
| `{DEBUG_LEVEL}` | **RS232 电平，不是 TTL**；`CON1`/`CON2` 不可直连 USB-TTL；TTL 侧须从扩展排针取 PA9/PA10 并共地 | P3 网络名 `RS232_RX1`/`RS232_TX1`/`RS232_RX2`/`RS232_TX2` + `ST3232` 收发器 | 图纸事实 + 器件常识推断 | 高 |
| `{DEBUG_BAUD}` | `115200` 8N1 | 工作流 0.1 表设定 | **非图纸事实**（工程约定） | — |
| `{SWDIO_PIN}` | `PA13 / JTMS-SWDIO`（pin 105） | P1 `PA13/JTMS-SWDIO105`；P1 `JTMS/SWDIO` 网络标签 | 图纸事实 | 高 |
| `{SWCLK_PIN}` | `PA14 / JTCK-SWCLK`（pin 109） | P1 `PA14/JTCK-SWCLK109`；P1 `JTCK/SWCLK` 网络标签 | 图纸事实 | 高 |
| `{SWO_PIN}` | `PB3 / JTDO / TRACESWO`（pin 133）；另有网络标签 `JTDO/SWO` | P1 `PB3/JTDO/TRACESWO/SPI3_SCK/I2S3_CK133`、`JTDO/SWO` | 图纸事实（引脚复用）；**是否已引到调试口：待人工确认** | 高 / 待人工确认 |
| `{NRST_PIN}` | `NRST`(pin 25)；`PB4 / JNTRST`(pin 134) 为 JTAG 复位，**不是**芯片 `NRST` | P1 `NRST25`、`PB4/JNTRST/SPI3_MISO134` | 图纸事实 | 高 |
| `{JTAG_DI_PINS}` | `PA15/JTDI`(110)、`PB4/JNTRST`(134)、`PB3/JTDO`(133) | P1 引脚表 | 图纸事实 | 高 |
| `{DEBUG_CONN}` | `J5 JTAG`，**20 针**：pin1 `Vref`、pin2 `Vsupply`、pin3 `nTRST`、pin5 `TDI`、pin7 `TMS`、pin9 `TCK`、pin11 `RTCK`、pin13 `TDO`、pin15 `nSRST`、pin17 `DBGRQ`、pin19 `DBGACK`；pin4/6/8/10/12/14/16/18/20 为 `GND`（**注意 pin2 是 `Vsupply` 而非 GND**） | P1 obj 69 引脚表 `Vref1`…`GND920J5JTAG` 与渲染图 | 图纸事实 | 高 |
| `{JTAG_SEL_JP}` | `JP6 JTAG_SEL`，10 针（2×5）；右侧引出 `SWDIO` / `SWCLK` | P1 `10JP6JTAG_SEL`、`SWDIOSWCLK`、`JNTRSTJTDIJTMSJTCKJTDOPB4PA13PA14PB3PA15` | 图纸事实 | 高 |
| `{RESET_CIRCUIT}` | `S1 RESET` + `C38 100nF` + `R26 10K` → `NRST`(25)；`R49 0R` → `J5` pin15 `nSRST`；`R40 1M` 与 Y2 相关 | P1 `S1RESET`、`C38100nF`、`R2610K`、`R490R`、`R401M` | 图纸事实 | 高 |
| `{BOOT0_JP}` | `JP8 BOOT0`，3 针（1=3V3 / 2→`R24 10K`→`BOOT0` pin138 / 3=GND）；**跳线帽默认位置待人工确认** | P1 `3JP8BOOT0`、`R2410K`、`BOOT0138` 与渲染图 | 图纸事实（结构）；默认状态未画 | 高 / 待人工确认 |
| `{BOOT1_JP}` | `JP9 BOOT1`，3 针（1=3V3 / 2→`R25 10K`→`BOOT1`/`PB2` / 3=GND）；`R25 10K` 为弱驱动，不会强拉 PB2，但须目视确认；**跳线帽默认位置待人工确认** | P1 `3JP9BOOT1`、`R2510K`、`BOOT1`、`PB2` 与渲染图 | 图纸事实（结构）；默认状态未画 | 高 / 待人工确认 |
| `{BEEP_JP}` | `JP5 BEEP`（蜂鸣器使能，图纸渲染为 2 针；针数与默认短接状态须目视确认） | P1 `JP5BEEP` 与渲染图 | 图纸事实（存在）/ 待人工确认（针数/状态） | 中高 |
| `{POWER_JP}` | `JP4 Power`（3 针，pin2 接 `5V`）；`JP7`（3 针） | P1 `3JP4Power`、`5V`、`JP7` | 图纸事实 | 高 |
| `{BOARD_SUPPLY}` | 3V3 数字/模拟分区滤波（`L2 10uH`、`R47`/`R48 10R`、`C23`~`C37`），`U2 LT1117-3.3`；`5V` 供蜂鸣器 | P1 `L210uH`、`U2LT1117-3.3`、`5V` 与渲染图 | 图纸事实 | 高 |
| `{SYSCLK_MHZ}` | `72` MHz（目标） | 工作流 0.1 表设定 | **非图纸事实**（工程目标） | — |
| `{JLINK_DEVICE}` | `STM32F103ZE` | 工作流 0.1 表 | **非图纸事实**（调试器参数） | — |
| `{SVD}` | `STM32F103xx.svd`（实测绝对路径见 `docs/env_check.md` 8.3） | 阶段 0 实测 | **非图纸事实**（工具链） | — |
| `{CB_EXT_AVAILABLE}` | `Cortex-Debug` 已安装（阶段 0 实测） | 阶段 0 实测 | **非图纸事实**（IDE 探测） | — |
| `{PWM_PATH}` | **A（TIM 更新中断 + GPIO 软件 PWM，不改硬件）** 为推荐值，READY 审阅可改选 B/C | 阶段 1 决策门（第 5 节） | 工程决策 | — |

---

## 2. 证据表：图纸逐字证据与核对结果

| # | 图纸文字 / 视觉证据（逐字） | 页码 | 支撑字段 |
| --- | --- | --- | --- |
| E1 | `STM32F103ZET6`（渲染图中央，文字层只到 `U1STM32F103`） | P1 | `{MCU}` |
| E2 | `Vdd_3144U1STM32F103`、`Vss_3143`、`PB2/BOOT148` | P1 | 144 脚、`{BUZZER_PIN}`、pin48 |
| E3 | `Y28MHz`、`C2120pF`、`C2220pF`、`OSC_IN23`、`OSC_OUT24` | P1 | `{HSE_MHZ}`、`{HSE_PIN}` |
| E4 | `Y132.768KHz`、`C1910pF`、`C2010pF`、`PC14-OSC32_IN8`、`PC15-OSC32_OUT9` | P1 | `{LSE_KHZ}`、`{LSE_PIN}` |
| E5 | `R2510K`、`PB2`、`PB2/BOOT148`、`3JP9BOOT1`、`BOOT1` | P1 | BOOT1 与 PB2 共网络 |
| E6 | `DZ1BEEP`、`5V`、`R391K`、`Q1NPN`、`JP5BEEP`、`BEEP` | P1 | `{BUZZER_DRIVER}` |
| E7 | `PB0/ADC12_IN8/TIM3_CH3/TIM8_CH2N46`、`PB1/ADC12_IN9/TIM3_CH4/TIM8_CH3N47`（`PB2/BOOT148` 无 TIM 字段） | P1 | `{BUZZER_TIM}`、`{PWM_FALLBACK_PIN}` |
| E8 | `PA9/USART1_TX/TIM1_TX/TIM1_CH2101`、`PA10/USART1_RX/TIM1_CH3102`、`PA2/USART2_TX36`、`PA3/USART2_RX37` | P1 | `{DEBUG_TX_PIN}`、`{DEBUG_RX_PIN}` |
| E9 | `PA13/JTMS-SWDIO105`、`PA14/JTCK-SWCLK109`、`PA15/JTDI/SPI3_NSS/I2S3_WS110`、`PB3/JTDO/TRACESWO/SPI3_SCK/I2S3_CK133`、`PB4/JNTRST/SPI3_MISO134` | P1 | SWD/JTAG 引脚 |
| E10 | `Vref1`、`Vsupply2`、`nTRST3`…`GND920J5JTAG`、`R490R`、`NRST25` | P1 | `{DEBUG_CONN}`、`{NRST_PIN}` |
| E11 | `10JP6JTAG_SEL`、`SWDIOSWCLK`、`3JP8BOOT0`、`3JP9BOOT1`、`3JP4Power`、`JP5BEEP` | P1 | 跳线 |
| E12 | `S1RESET`、`C38100nF`、`R2610K`、`L210uH`、`U2LT1117-3.3` | P1 | `{RESET_CIRCUIT}`、`{BOARD_SUPPLY}` |
| E13 | `Title Number Revision Size B Date:28-Apr-2009 Sheet of File:F:\STM32-X5.Ddb`（P1）；P2/P3 为 `Size A4` | P1/P2/P3 | 板卡版本复核项 #9 |
| E14 | `GND15U5ST3232`、`RS232_RX1RS232_TX1PA9PA10`、`CON1UART1`、`CON2UART2`、`RS232_RX2RS232_TX2`、`U1TX/U1RX/U2TX/U2RX` | P3 | `{DEBUG_LEVEL}`、`{DEBUG_UART_CONN}` |
| E15 | `RE2U7MAX485`、`485_DIRPF11PB11PB10PB9PB8`、`U6MCP2551`、`CN2CAN` | P3 | USART3 已被 RS485 占用，不适合做 Debug 口 |
| E16 | `10CN1CON10`、`PC0PC1PC2PB0PB1PA4PA5`、`AIN0/AIN1/AIN2/PWM0/PWM1/DAC0/DAC1`；渲染图 CN1 pin1–10：1=PC0、2=PC1、3=PC2、4=PB0(PWM0)、5=PB1(PWM1)、8=PA4(DAC0)、9=PA5(DAC1) | P3 | `{PWM_FALLBACK_PIN}` |
| E17 | `U8 M29W128FH`、`U9 IS61LV51216`、`U10 NAND512`、`FSMC_NE2/NE3/NE4`、`FSMC_NWAIT`（渲染图） | P2 | 板载外扩存储器（非 MCU 内部 Flash/SRAM） |
| E18 | P1 渲染图左下：`BEEP` → `JP5` → `R39 1K` → `Q1(NPN)` 基极，发射极接地，集电极经 `DZ1` 到 `5V`；`JP8`/`JP9` 为 3 针，pin2 分别经 `R24 10K`/`R25 10K` 到 `BOOT0`/`BOOT1` | P1 | 有效电平推断、BOOT 冲突复核 |

**与工作流 1.1 基线的差异（只修正证据不足项，不改写结论）**：

1. `STM32F103ZET6` 由渲染图确认（文字层只到 `U1STM32F103`），基线「P1 `U1 STM32F103ZET6`」成立。
2. J5 的 **pin2 是 `Vsupply`，不是 GND**；基线「偶数脚 GND」不严格准确，已按 E10 修正为「pin2=Vsupply，其余偶数脚 GND」。
3. `Q1` 图纸只写 `NPN`，**未给具体三极管型号**；基线也未提供型号，故列入待人工确认。
4. `{MCU_CORE}` / `{MCU_FLASH}` / `{MCU_SRAM}` / 封装名 / 蜂鸣器本体型号：图纸均未写出，按硬约束列为「待人工确认」，不以型号后缀反推；蜂鸣器**类型**已由 11-12 实测确认为有源（附录 B.2），不再是待确认项。
5. CN1 的 PWM0/PWM1 对应 **pin4 / pin5**（渲染图逐针核对），基线只写网络名，本次补齐物理针位；CN1 pin6/7/10 无网络标签，待人工确认。

---

## 3. 「待人工确认」清单与实测判定方法

| # | 字段 | 为什么不能写结论 | 看哪里 / 怎么量 | 预期 / 判定方法 |
| --- | --- | --- | --- | --- |
| D1 | `{MCU_CORE}` / `{MCU_FLASH}` / `{MCU_SRAM}` | 图纸未标注 MCU 内部资源，严禁按型号后缀臆造 | 查 ST 官方 `STM32F103ZET6` 数据手册；或用 J-Link 读 Flash 容量寄存器 / SCB CPUID 核对内核 | 以官方文档数值为准，回填本表 |
| D2 | 封装名 `LQFP144` | 图纸只写 1–144 脚号，未写封装字符串 | 目视芯片丝印与封装外形；数引脚/测引脚间距 | 以实物丝印为准 |
| D3 | `Q1` 三极管型号 | 图纸只标 `Q1NPN` | 断电目视 Q1 丝印；万用表二极管档测 B/E、B/C | 记录实际型号，确认是 NPN |
| D4 | 蜂鸣器**类型** | ✅ **已闭环 11-12**：GDB 直写 PB2 恒高 15 秒，用户回报持续鸣叫（附录 B.2） | 结论：**有源蜂鸣器**。本体型号仍待人工确认（D4b：断电目视丝印，照片上圆形器件被白色贴纸盖住） | 已实测确认 |
| D5 | `{BUZZER_ACTIVE_LEVEL}` | ✅ **已闭环 11-12**：GDB 分别置 PB2=1 / 0，`ODR` 读回 `0x14`→`0x10`，鸣叫窗口与置 1 的 15 秒一致（附录 B.2） | 结论：**高电平有效**，已回填第 1 节 | 已实测确认 |
| D6 | `DZ1` 与蜂鸣器的确切连接关系、5V 供电路径 | 文字层只有 `DZ1BEEP`/`5V` 标签，拓扑靠视觉推断 | 断电万用表通断档量 `DZ1` 两端、`JP5` 两端、5V 到蜂鸣器 | 画出实际串并联关系后回填 |
| D7 | `JP5` 针数与跳线帽默认状态 | 图纸不画跳线帽，渲染图为 2 针但不清晰 | 断电目视跳线帽位置；通断档量 JP5 两端 | 必须短接，否则蜂鸣器永远不响 |
| D8 | `JP8` / `JP9` 跳线帽默认状态 | 图纸只给 3 针结构 | 目视；万用表量 pin2 对 3V3 / GND | 调试需 `BOOT0=0`；`BOOT1` 不得把 PB2 强拉到固定电平 |
| D9 | `J5` pin1 `Vref` 电压值 | 图纸只标 `Vref`/`Vsupply` 网络，未标电压值 | 上电万用表量 J5 pin1 / pin2 对 GND | 应为 3V3（工作流假设，须实测） |
| D10 | `{SWO_PIN}` 是否引到调试口 | 图纸有 `PB3/JTDO/TRACESWO` 与 `JTDO/SWO` 标签，但引出路径未核 | 目视 `JP6` 与 J5 走线；通断档量 PB3 到调试口 | 记录是否可用 SWO；本项目 Debug 输出走串口，不依赖 SWO |
| D11 | CN1 pin6/7/10 用途 | 无网络标签 | 目视丝印 + 万用表通断档量 | 记录后回填 |
| D12 | `{PWM_OTHER_TIM_PINS}` 是否引到扩展口 | P1 引脚表有 TIM 复用，但 CN1 未标这些网络 | 目视扩展排针丝印 + 通断档量 PA6/PA7/PB6–PB9 | 仅把「已引出且无冲突」的列入硬件 PWM 备选 |

---

## 4. 人工复核清单（9 项 · 验收门）

> 口径：**硬约束 2 优先**。AI 无法目视实物，凡需现场观察的项一律写「待人工确认 + 判定方法」，不臆造结论；用户回传照片/描述后补结论。#5/#6 的可观测证据由 GDB 寄存器实测给出，听觉判断必须由用户回报。

| # | 复核项 | 看哪里 / 怎么量 | 预期结果 | 当前结论 |
| --- | --- | --- | --- | --- |
| 1 | `JP5 BEEP` 是否短接 | 断电目视跳线帽；通断档量 JP5 两端 | 短接，否则蜂鸣器永远不响 | **待人工确认**（图纸：`JP5 BEEP` 存在，P1） |
| 2 | `JP6 JTAG_SEL` 模式 | 对照板上丝印与 P1 右下（E11） | 设为 J-Link 所需模式，`SWDIO`/`SWCLK` 有效 | **待人工确认**（图纸：10 针、引出 SWDIO/SWCLK，P1） |
| 3 | `JP8 BOOT0` | 目视跳线帽；万用表量 pin2 | `BOOT0=0`（主 Flash 启动），单步调试必需 | **待人工确认**（图纸：3 针，pin1=3V3 / pin3=GND，P1） |
| 4 | `JP9 BOOT1` 与 PB2 冲突 | 目视跳线帽；万用表量 pin2 对 PB2；GDB 实测前确认 | 不得把 PB2 强拉到固定电平；`R25 10K` 为弱驱动，推挽可覆盖 | **图纸事实已确认**：`R25 10K` 串接，非强拉（P1 E5/E18）；跳线帽位置 **待人工确认** |
| 5 | **蜂鸣器有源 / 无源** | GDB 直写 PB2 恒高（附录 A），用户听声音 | 持续鸣叫＝**有源**；仅上电瞬间「嗒」一声＝**无源** | ✅ **实测结论：有源蜂鸣器**（11-12 PB2=1 保持 15 秒，用户回报「我听到了 15 秒的持续鸣叫」，附录 B.2） |
| 6 | **有效电平** | 同上，分别置 PB2=1 与 PB2=0，读回 `ODR` | 确认哪一种响，回填 `{BUZZER_ACTIVE_LEVEL}` | ✅ **实测结论：高电平有效**（`ODR` `0x14`→`0x10`，鸣叫窗口与 PB2=1 的 15 秒一致，置 0 后停止；附录 B.2） |
| 7 | Debug 串口接线方式 | 目视 `CON1` DB9 与扩展排针 | 用 `CON1` 必须配 **RS232-USB** 线；USB-TTL 须从扩展排针取 `PA9/PA10` TTL 并共地 | **图纸结论已锁定**：`CON1 UART1` 经 `U5 ST3232` 为 **RS232 电平，不是 TTL**（P3 E14）；实际接线方式 **待用户回传** |
| 8 | J-Link 接口匹配 | 目视 `J5` 20pin 与 J-Link 线、转接板 | 针距/转接板匹配；`Vref` 接 3V3；`GND` 共地 | **图纸结论已锁定**：`J5` 20 针（P1 E10）；针距/Vref 电压 **待人工确认**（D9） |
| 9 | 板卡版本一致性 | 板子丝印 vs 图纸 `F:\STM32-X5.Ddb` / `28-Apr-2009` | 版本不同则以实物丝印与万用表实测为准并重写映射表 | **图纸基线已记录**（E13，**所有 3 页**）；实物丝印 **待人工确认** |

---

## 5. 阶段 1 决策门：`{PWM_PATH}`

**结论（READY 推荐值）：路径 A —— 定时器更新中断 + GPIO 软件 PWM，不改硬件。**

| 路径 | 是否改硬件 | 适用条件 | 效果 / 代价 |
| --- | --- | --- | --- |
| **A（推荐）** | **不改** | 任意蜂鸣器类型 | TIM 更新中断周期性翻转 PB2；有源蜂鸣器得到「滴滴」节奏与占空比控制，无源蜂鸣器可变频鸣叫。READY 审阅可改选 B/C |
| B | 飞线 | 断开 `JP5` 的 PB2 侧，从 `CN1` pin4 `PWM0`(PB0/TIM3_CH3) 引到蜂鸣器驱动输入 | `HAL_TIM_PWM_Start` 纯硬件 PWM，ARR/CCR 精确控制；需动烙铁/飞线 |
| C | 焊接换件 | 确认为有源蜂鸣器且希望变频改变音调 | 换无源蜂鸣器 + 路径 B |

**硬约束语义**（工作流 273 行）：**有源蜂鸣器无法通过 PWM 变频改变音调**，PWM 只能控制通断节奏与占空比；只有无源蜂鸣器能靠频率变调。阶段 4 按 `{BUZZER_TYPE}` 与 `{PWM_PATH}` 分支，避免生成与硬件不符的业务行为。

**选定依据**：PB2 无 TIMx_CHy 复用（P1 E7，高置信度），A 是唯一无需改硬件的路径，且对有源/无源都成立；B/C 需硬件改造，留待 READY 审阅决定。

---

## 6. 附录 A：PB2 寄存器实测方案（不编译工程）

### A.1 工具与硬约束（阶段 0 实测绝对路径）

| 项 | 实测值 | 来源 |
| --- | --- | --- |
| `JLinkGDBServerCL.exe` | `C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe`（590288 字节） | `docs/env_check.md` 8.3 |
| `arm-none-eabi-gdb.exe` | `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-gdb.exe`（`15.2.90.20241130-git`） | `docs/env_check.md` 8.3 |
| `STM32F103xx.svd` | `C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd`（867312 字节） | `docs/env_check.md` 8.3 / 8.4 |

- **不调用** `cmake` / `ninja` / `arm-none-eabi-gcc`，不 `file`、不 `load` 任何 `.elf`，不烧录、不改 option bytes、不启用读保护。
- J-Link 参数**不得凭记忆编造**（工作流 559 行；阶段 0 明确未执行，`docs/env_check.md` 8.4）。本次实测：`--help` 与 `-?` **都不是合法参数**（打印 `WARNING: Unknown command line parameter ...` 后仍按默认设置尝试连接）；改从 `JLinkGDBServerCL.exe` 二进制提取到 `-device` / `-if` / `-speed` / `-port` / `-singlerun` / `-nogui` / `-select` / `-listprobes` 等，并由启动回显确认参数被正确解析（证据见附录 B）。默认 GDB 端口 `2331` 亦由回显实测。
- 寄存器地址与位定义以 SVD 为准，下列值已逐行读回核对。

### A.2 SVD 寄存器依据（逐项读回，非记忆）

| 用途 | 地址 / 位定义 | SVD 依据 |
| --- | --- | --- |
| RCC 基址 | `0x40021000` | 1586/1589 行 |
| `RCC_APB2ENR` | `0x40021018`（`addressOffset 0x18`），`IOPBEN` = **bit3**（开 GPIOB 时钟） | 2239–2266 行 |
| GPIOB 基址 | `0x40010C00`（`derivedFrom="GPIOA"`） | 3539–3540 行 |
| `GPIOB_CRL` | `0x40010C00`（`addressOffset 0x0`，复位 `0x44444444`）；`MODE2` = bitOffset **8**、width 2；`CNF2` = bitOffset **10**、width 2 | 2663–2709 行 |
| `GPIOB_ODR` | `0x40010C0C`（`addressOffset 0xC`），`ODR2` = bit2 | 3003–3007 行 |
| `GPIOB_BSRR` | `0x40010C10`（`addressOffset 0x10`，只写），`BS2` = bit2 → `0x00000004` 置高 | 3111–3115 行 |
| `GPIOB_BRR` | `0x40010C14`（`addressOffset 0x14`，只写），`BR2` = bit2 → `0x00000004` 置低 | 3315–3319 行 |

PB2 推挽输出配置：`MODE2 = 0b11`（50 MHz）+ `CNF2 = 0b00`（通用推挽）→ 位区值 `0x00000300`，采用**读—改—写**：`CRL = (CRL & 0xFFFFF0FF) | 0x00000300`，避免破坏 PB0/PB1/PB3 配置。

### A.3 GDB 命令序列（端口 `2331` 已由 GDB Server 启动回显实测）

```gdb
set confirm off
set pagination off
set mem inaccessible-by-default off
target remote localhost:2331            # GDB Server Listening port 回显实测
monitor halt                            # 停内核，防止 Flash 内既有固件回写 GPIOB 配置

# 1) 开 GPIOB 时钟（IOPBEN bit3）
set $apb2 = *(volatile unsigned int*)0x40021018
set *(volatile unsigned int*)0x40021018 = ($apb2 | 0x00000008)
x/1xw 0x40021018                        # 期望 bit3 = 1

# 2) PB2 配成推挽输出（读—改—写）
set $crl = *(volatile unsigned int*)0x40010C00
set *(volatile unsigned int*)0x40010C00 = (($crl & 0xFFFFF0FF) | 0x00000300)
x/1xw 0x40010C00                        # 期望 bits[11:8] = 0b0011

# 3) PB2 = 1，持续观察声音（判定有源/无源）
set *(volatile unsigned int*)0x40010C10 = 0x00000004
x/1xw 0x40010C0C                        # 期望 bit2 = 1

# 4) PB2 = 0，对照静音（判定有效电平）
set *(volatile unsigned int*)0x40010C14 = 0x00000004
x/1xw 0x40010C0C                        # 期望 bit2 = 0
```

**收尾**：保持 PB2=0 静音 → `monitor reset` 或断开 `target`，不烧录、不写 Flash。

### A.4 硬件前置与禁忌（阶段 0 归档计划 87–88 行 / `docs/env_check.md` 第 5 节）

`JP5 BEEP` 短接；`JP8 BOOT0=0`；`JP6 JTAG_SEL` 为 SWD 可用模式；`J5` pin1 `Vref`=3V3、GND 共地；板卡上电；**禁止带电插拔**；写 GPIO 前确认 `JP9 BOOT1` 未把 PB2 强拉（E5：`R25 10K` 弱驱动）；5V 仅供蜂鸣器；`CON1/CON2` 为 RS232 电平，不可直连 USB-TTL。

### A.5 判据与失败回退

| 现象 | 判定 / 动作 |
| --- | --- |
| PB2=1 持续鸣叫，PB2=0 静音 | 有源蜂鸣器、高电平有效 → 回填 `{BUZZER_TYPE}` / `{BUZZER_ACTIVE_LEVEL}` |
| PB2=1 仅上电瞬间「嗒」一声 | 无源蜂鸣器 → 变频鸣叫才有意义，重新评估 B/C |
| `APB2ENR` 读回 bit3≠1 | GPIOB 时钟门控未开，检查 `monitor halt` 是否生效、是否被复位 |
| `CRL` 读回不符 | 检查 Flash 内既有固件是否回写；重新 halt 后再写 |
| `ODR` 读回符合但无任何声音 | 按 #1 `JP5`、#7 供电、`DZ1`/Q1 拓扑、`JP9` 冲突逐项排查 |
| J-Link 连不上 | 决策树：跳线 → `Vref` → SWD 引脚是否被复用 → `BOOT0`；连接失败只记退出码与首行错误，不盲目重试 |
| 无用户听音回报 | #5/#6 一律保持「待人工确认」，**不得臆造结论，不得进入阶段 4** |

---

## 7. 附录 B：寄存器实测记录

> 本节只记录**可观测的终端输出**与**用户回报原文**，不写推断。

### B.1 探测与连接实测（2026-11-11）

| # | 动作 | 可观测输出（逐字要点） | 结论 |
| --- | --- | --- | --- |
| 1 | `JLinkGDBServerCL.exe --help` | `WARNING: Unknown command line parameter --help found.`；回显 `GDB Server Listening port: 2331`、`Target device: Unspecified`、`Target interface: JTAG`、`Target interface speed: 4000kHz`；停在 `Connecting to J-Link...`（20 s 超时终止进程） | `--help` 非法；默认端口 2331 实测确认 |
| 2 | `JLinkGDBServerCL.exe -?` | `Could not connect to J-Link.` / `Connecting to J-Link failed. Connected correctly?` / `Please check power, connection and settings.` | `-?` 非法；连接失败 |
| 3 | 从 exe 二进制提取可打印参数字符串 | `-device` `-if` `-speed` `-port` `-select` `-listprobes` `-singlerun` `-silent` `-halt` `-nohalt` `-nogui` `-endian` `-jtagconf` `-powertarget` 等 | 参数写法有实测依据，非记忆 |
| 4 | `Get-PnpDevice` 查 `VID_1366` | 两条 `J-Link driver`（`USB\VID_1366&PID_0101\001245593416`、`...\000099999994`），两者 `Problem = CM_PROB_PHANTOM` | **幻影设备 = J-Link 当前未插入** |
| 5 | 以 `-device STM32F103ZE -if SWD -speed 4000 -port 2331 -singlerun -nogui` 启动 | 回显 `Command line: -device STM32F103ZE -if SWD -speed 4000 -port 2331 -singlerun -nogui`、`Target device: STM32F103ZE`、`Target interface: SWD`；随后 `Connecting to J-Link failed. Connected correctly?`、`GDBServer will be closed...`；stderr `Could not connect to J-Link. Please check power, connection and settings.` | 参数解析正确，**硬件连接失败** |

### B.2 11-12 实测记录（成功，已闭环）

**前置（只读核验）**：`docs/hardware_pins.md` 与 0.1 表两行回填均存在；`Get-PnpDevice` 有一条 `VID_1366` `Status OK / CM_PROB_NONE`；GDB Server 回显 `J-Link is connected.`、`Target voltage: 3.31 V`、`Halting core... Connected to target`、`Listening on TCP/IP port 2331`。

**GDB 会话（`-batch -x`，未 `file`、未 `load` 任何 elf）逐字读回**：

| 步骤 | 命令 | 读回值 | 判定 |
| --- | --- | --- | --- |
| 开 GPIOB 时钟 | `set *(unsigned int*)0x40021018 = (... \| 0x8)` → `x/1xw 0x40021018` | `0x0000400C` | bit3 `IOPBEN`=1 |
| PB2 配推挽输出 | `CRL` 读改写 `& 0xFFFFF0FF \| 0x300` → `x/1xw 0x40010C00` | `0x44484344` | `bits[11:8]=0b0011`（MODE2=11 / CNF2=00） |
| PB2=1，保持 15 s | `set *(unsigned int*)0x40010C10 = 0x4` → `x/1xw 0x40010C0C` | `0x00000014` | bit2=1，输出高 |
| PB2=0，静音收尾 | `set *(unsigned int*)0x40010C14 = 0x4` → `x/1xw 0x40010C0C` | `0x00000010` | bit2=0，输出低 |

Server 关闭回显：`GDB closed TCP/IP connection`、`Restoring target state and closing J-Link connection...`、`Shutting down...`；收尾核验 `NO_SERVER_LEFT` / `PORT_2331_FREE`。GDB 的 `warning: No executable has been specified ...` 仅表示未加载 elf，四条读回全部有效。

**用户回报原文（11-12）**：「我听到了 15 秒的持续鸣叫」。

**结论（按 A.5 判据）**：

- 持续鸣叫 → **有源蜂鸣器** → `{BUZZER_TYPE}` 锁定为「有源」。
- 鸣叫窗口与 PB2=1 保持的 15 秒一致，置 0（`ODR` `0x10`）后停止 → **高电平有效** → `{BUZZER_ACTIVE_LEVEL}` 锁定为「高电平鸣叫」。
- 有源蜂鸣器不能靠 PWM 变频改变音调（工作流 273 行），`{PWM_PATH}=A` 保持成立，B/C 无需启用。
- **未编译工程**：全程未调用 `cmake`/`ninja`/`arm-none-eabi-gcc`，未生成固件或构建产物，未烧录、未改 option bytes。
- 11-11 曾因 J-Link 未插入（`CM_PROB_PHANTOM`）阻塞，见 B.1；该记录保留为历史，不再代表当前状态。
- 附带发现：halt 时 `pc = 0x1FFFF3B6`（系统存储器 / bootloader 区），提示 `JP8 BOOT0` 当前可能非 0；不影响本次 GPIO 实测，阶段 5 从 Flash 单步前须按 #3 复核该跳线。

### B.3 复测命令（已执行完毕，留作复现；仍不编译工程）

```powershell
# 1) 断电接好 J-Link 到 J5（20pin）、确认 JP6=SWD、JP8 BOOT0=0、JP5 BEEP 短接，再上电
# 2) 启动 GDB Server（参数已由 B.1 #5 回显实测）
Start-Process -FilePath "C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe" `
  -ArgumentList '-device','STM32F103ZE','-if','SWD','-speed','4000','-port','2331','-singlerun','-nogui' `
  -NoNewWindow
# 3) 另开终端启动 GDB，不加载任何 elf
& "C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-gdb.exe"
```

该命令序列已于 11-12 执行完毕：GDB 逐条写入 + `x/1xw` 读回值、用户听音回报与结论见 **B.2**；`{BUZZER_TYPE}` / `{BUZZER_ACTIVE_LEVEL}` 及第 4 节 #5/#6 均已回填。此处保留命令仅用于复现；若需复测，同样回报写入值、读回值与听音结果即可。

> 注意：`monitor halt` / `monitor reset` 是否受当前固件与连接状态支持，须在连上后以 GDB 实际回显为准，不预设成功。

---

## 8. READY 审阅物与验收门

**审阅物**：第 1 节映射表 + 第 4 节 9 项复核 + 第 5 节 `{PWM_PATH}`。

**验证点**：每个字段有证据页码与置信度；图纸事实与电路推断分栏；无法确认项全部写「待人工确认」并给实测判定方法；`{PWM_PATH}` 已选定；未产生任何编译产物。

**验收标准**：「映射表已锁定，共 N 项高置信、M 项已实测确认、0 项臆造」；第 5、6、7 项有实测/图纸结论；把映射表贴回给 AI 能复述 `{BUZZER_PIN}`、`{DEBUG_USART}`、`{SWDIO_PIN}` 三项且与实物一致。

**失败回退**：PDF 文字层缺失 → 渲染 PNG → OCR/视觉 → 万用表通断档；蜂鸣器类型与有效电平无法确认 → 不进入阶段 4，用 GDB 寄存器实测或人工回报补结论；板卡版本不符 → 冻结推断项、仅保留实测项、重写映射表。

### 验收门当前状态（2026-11-12 更新）

| 项 | 状态 |
| --- | --- |
| 映射表每字段有证据页码 / 置信度 / 事实或推断分类 | ✅ 高置信字段 30+ 项，0 项臆造；图纸未写出的项全部列第 3 节并给判定方法 |
| 9 项复核逐项写清「看哪里 / 怎么量 / 预期结果」 | ✅ 9/9；#5/#6 已于 11-12 实测闭环，图纸可判定的 #4/#7/#8/#9 已锁定结论 |
| `{PWM_PATH}` 已选定 | ✅ A（实测确认有源蜂鸣器后仍适用；READY 可改选 B/C） |
| 未编译工程 / 未生成固件或构建产物 | ✅ 全程未调用编译器，未 `file`/`load` 任何 elf，未烧录 |
| 第 5、6 项有实测结论 | ✅ **已达成**：有源蜂鸣器 / 高电平有效，GDB 读回值 + 用户听音回报见附录 B.2 |
| 三关键字段可复述 | ✅ `{BUZZER_PIN}` = `GPIOB`/`GPIO_PIN_2`（PB2，pin 48）、`{DEBUG_USART}` = `USART1`、`{SWDIO_PIN}` = `PA13`（pin 105） |

> READY 结论：**映射表已锁定，共 30+ 项高置信、2 项已实测确认、0 项臆造**；阶段 1 验收门全部达成，可进入阶段 2。第 3 节剩余项（跳线目视、板卡版本、器件型号、CN1 引脚等）为非阻塞的补充核对，不影响阶段 1 通过；`JP8 BOOT0` 因 `pc=0x1FFFF3B6` 的发现，建议在阶段 5 前优先确认。
