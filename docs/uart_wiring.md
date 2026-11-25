# 阶段 3：串口终端配置清单与接线对照表

> 项目：STM32F103ZET6 红牛开发板，USART1 Debug 输出，J-Link SWD 单步调试
> 生成日期：2026-11-12 ｜ 平台：Windows + PowerShell ｜ 开发前端：CodeBuddy IDE
> 只读输入：`@STM32_Buzzer_PWM_Workflow.md` 阶段 3（372–438 行）、`@docs/hardware_pins.md`（锁定版唯一硬件真相源）、`@docs/env_check.md`、`@docs/stage2_build_report.md`、`@docs/stage2_manual_notes.md`、`@docs/clock_tree.md`、`@Core/Src/main.c`、`@Core/Inc/main.h`、`@Core/Inc/debug_log.h`、`@Core/Src/debug_uart.c`
> **边界声明**：本文以接线与终端配置说明为主，并登记 B 段实测结果；**全程无改跳线、改驱动动作**（跳线由用户人工改动）。2026-11-12 最终状态：用户已把 `BOOT0` 改到 `2-3`，J-Link 只读 halt 实测 `PC=0x08004FE6` 确认从 **User Flash** 启动，`CON1` 侧已确认为 RS232 电平，且用户明确回复「可以烧录」；**两个 B 前置均已满足，B 已执行完毕** —— 固件已烧录，COM3 连续 3 次读到一致启动日志，`main.c:171` 断点 + `next` 单步验证通过，验收 ⑥⑦ 均已达成。第 4 节为**实测输出**（不再是格式示例），完整逐字节证据见 `docs/stage3_report.md` 第 6.3 节。

---

## 0. 本阶段锁定的硬件事实（不得重新猜测）

| 字段 | 锁定值 | 依据 |
| --- | --- | --- |
| `{MCU}` | `STM32F103ZET6` | `@docs/hardware_pins.md` 38 行 |
| HSE / SYSCLK | `8 MHz` → PLL×9 → `72 MHz`（APB1 36 / APB2 72） | 锁定版 42、74 行；`@docs/clock_tree.md` 第 1 节；**运行态实测 `72000000Hz`**（第 4 节） |
| `{DEBUG_USART}` | `USART1` | 锁定版 55 行 |
| `{DEBUG_TX_PIN}` / `{DEBUG_RX_PIN}` | `PA9`(LQFP144 pin 101) / `PA10`(pin 102)，网络名 `U1TX` / `U1RX` | 锁定版 56、93 行 |
| `{DEBUG_BAUD}` | `115200 8N1`，无流控 | 锁定版 60 行；`Core/Inc/main.h:18` `DEBUG_USART_BAUDRATE` |
| `{DEBUG_LEVEL}` | **RS232 电平，不是 TTL** | 锁定版 59 行、P3 E14；用户已确认 |
| 电平转换芯片 | `U5 ST3232` + `C39`~`C42` | 锁定版 57、99 行 |
| DB9 连接器 | `CON1 UART1`（USART1）、`CON2 UART2` | 锁定版 58 行；本阶段只用 **CON1** |
| PC 侧串口 | `USB Serial Port (COM3)`，FTDI `VID_0403/PID_6001`，`Status OK` | `@docs/env_check.md` 10.1 / 351、369 行；烧录时实测可打开并稳定收发 |
| 已人工确认的接线 | **`CON1` DB9 配 RS232-USB 线** | `@docs/env_check.md` 341、352 行（阶段 1 复核清单第 7 项） |
| SWD | `PA13`(pin105) / `PA14`(pin109)，全程不得复用为 GPIO | 锁定版 61–62 行 |
| 蜂鸣器 | `BEEP/BOOT1` = `PB2`(pin48)，有源、高电平有效，PB2 无 TIM 通道，`{PWM_PATH}=A` | 锁定版 47、50–52、78 行 |

> **手册补充风险（不改写锁定版）**：`@docs/stage2_manual_notes.md` M6 提到手册新增跳线 `JP12`，`Default setting: not fitted`，且手册 CN1 针序与锁定版不同；本阶段**AI 未改任何跳线**，也不据手册改接线。`JP8`/`JP9` 与 `BOOT0`/`BOOT1` 的编号对应在手册与原理图间相反（M3/M4），按硬约束不改写锁定版；`BOOT0=0` 已由只读 halt 与烧录成功实测闭环，不依赖编号对照。

---

## 1. 串口终端配置清单

| # | 配置项 | 必须取值 | 说明 / 依据 |
| --- | --- | --- | --- |
| 1 | 端口 | `COM3` | 阶段 0 实测 `USB Serial Port (COM3)`，FTDI `VID_0403/PID_6001`；打开前可用只读命令 `mode COM3` 或 `Get-PnpDevice -Class Ports` 复核 |
| 2 | 波特率 | `115200` | `Core/Inc/main.h:18`；`USARTDIV = 72 000 000 / (16 × 115 200) = 39.0625`，`BRR = 0x271`（`@docs/clock_tree.md` 24 行）。波特率依赖 HSE 8 MHz 与 PLL×9，乱码先查时钟 |
| 3 | 数据位 | `8` | `UART_WORDLENGTH_8B`（`main.c` `MX_USART1_UART_Init`） |
| 4 | 停止位 | `1` | `UART_STOPBITS_1` |
| 5 | 校验位 | `None` | `UART_PARITY_NONE` |
| 6 | 流控 | `None`（关闭 RTS/CTS 与 XON/XOFF） | `UART_HWCONTROL_NONE`；勾选硬件流控会导致无输出 |
| 7 | 换行处理 | `CRLF`（`\r\n`） | 日志宏固定以 `\r\n` 结束（`Core/Inc/debug_log.h`）；终端建议勾选「Implicit CR in every LF」/「自动换行」，避免阶梯状显示 |
| 8 | 字符编码 | `UTF-8` / `ASCII`；逐字节核对用 `Latin1`(CP28591) | 本工程日志内容全部为 ASCII（无中文、无控制字符、无敏感信息）；实测按 Latin1 单字节读取，避免多字节解码掩盖杂散字节 |
| 9 | 本地回显 | 关闭 | 板卡不做接收回显，开启本地回显会让单行日志看起来重复 |
| 10 | DTR / RTS | 保持无效 | 避免误触发板卡复位或电平变化 |
| 11 | 推荐终端软件 | `PuTTY` / `Tera Term` / `MobaXterm` / `SecureCRT` | 任选支持 COM 口、115200 8N1、CRLF 显示的终端；也可使用任意等价串口终端 |
| 12 | 打开方式 | 串口（Serial），不是 Telnet/SSH | 连接类型选 Serial，填 `COM3` 与 `115200` |

**只读复核命令（PowerShell，不打开端口收发数据）**：

```powershell
Get-PnpDevice -Class Ports | Select-Object FriendlyName, InstanceId, Status
mode COM3
```

**已实测的终端行为**：`115200/8N1/None/CRLF` 下每次复位稳定收到一行日志；另有 1 个复位瞬间前导字节 `0xFC`（见第 4 节），终端可忽略首字节，或以 `[` 作为行起始判据。

---

## 2. 接线对照表（DB9 / 扩展排针 / USB-TTL 三种情况）

| 方案 | 板侧接口 | 线缆 | PC 侧 | 电平域 | 结论 | 关键说明 |
| --- | --- | --- | --- | --- | --- | --- |
| **① 已实测可用：CON1 + RS232-USB 线** | `CON1 UART1`（DB9） | **RS232-USB 线**（枚举为 `COM3`，FTDI `VID_0403/PID_6001`） | USB | 板侧 `U5 ST3232` 输出 **RS232 电平**，线缆自行完成 RS232↔USB | ✅ **唯一人工确认且本阶段实测输出正常的方案** | 与 `@docs/env_check.md` 341、352 行一致；`USART1=PA9/PA10` 经 ST3232 到 CON1；无需动任何跳线 |
| ② 备选：扩展排针取 TTL | 扩展排针上的 `U1TX`(PA9) / `U1RX`(PA10) 与 `GND` | **USB-TTL（3.3 V TTL 电平）** | USB | MCU 侧 **TTL/UART** | ⚠️ 需人工确认扩展排针实际针位与丝印后才可用 | 必须 `TX↔RX` 交叉、必须**共地**；只接 3.3 V TTL，禁止 5 V；不得同时把 CON1 的 RS232 侧并进来 |
| ③ **禁止**：CON1 直连 USB-TTL | `CON1 UART1`（DB9） | USB-TTL | USB | RS232 ↔ TTL **混接** | ❌ **绝对禁止** | `U5 ST3232` 输出约 ±3~15 V RS232 电平，直连 USB-TTL 会损坏 TTL 串口/MCU；且极性/电平都不匹配，必然无输出 |

**TX/RX 交叉与共地**：

- 方案 ①：DB9 侧由 RS232-USB 线按标准 DCE/DTE 定义完成交叉，用户只需确认线缆是「RS232-USB」而非「USB-TTL」。
- 方案 ②：PA9 是 MCU 的 **TX**，接 USB-TTL 的 **RXD**；PA10 是 MCU 的 **RX**，接 USB-TTL 的 **TXD**；**GND 必须共地**。
- 任何方案都需确认共地；`@docs/env_check.md` 164、166 行与 `@docs/hardware_pins.md` 59、146、228 行均把「不共地 / RS232 与 TTL 混接」列为硬件禁忌。

> **电平硬约束**：**RS232 电平与 TTL 电平不可混接，必须共地。** 板上 `CON1`/`CON2` 经 `U5 ST3232` 已是 RS232 电平，绝不能直连 USB-TTL；要用 USB-TTL，只能从扩展排针取 `PA9/PA10` 的 TTL 侧。改动接线前必须给板卡断电（`@docs/env_check.md` 163 行）。

---

## 3. 日志行为（代码层已确定，运行态已验证）

- `Core/Src/debug_uart.c` 用 `_write` 把 `printf` 重定向到 `huart1`，阻塞式 `HAL_UART_Transmit`，`setvbuf(..., _IONBF, 0)` 关闭缓冲 → 每行即时发送，单步时日志与执行顺序一致（第 5 节 ⑦ 已实测）。
- `Core/Inc/debug_log.h` 提供 `LOG_ERROR/LOG_WARN/LOG_INFO/LOG_DEBUG`，编译期等级开关；Debug 构建等级 4、Release 等级 2。
- 启动日志只在 `main()` 的四个频率快照之后输出 **1 行**，主循环不打印、TIM2 更新中断不打印、任何 ISR 不调用 `printf`。
- 日志内容：固件版本、编译时间（`__DATE__ __TIME__`）、MCU 型号、`SYSCLK` 实际值（取 `g_sysclk_hz = HAL_RCC_GetSysClockFreq()`）、`USART1/115200-8N1`、蜂鸣器引脚与 `PWM_PATH=A` 摘要；全部 ASCII、无敏感信息。

---

## 4. 实测启动日志（2026-11-12，COM3 真实捕获）

```text
[INF] STAGE3 FW=0.3.0 BUILD=Nov 12 2026 13:54:26 MCU=STM32F103ZET6 SYSCLK=72000000Hz UART=USART1/115200-8N1 BEEP=PB2(pin48)/active-high PWM_PATH=A\r\n
```

- `BUILD=` 后的日期时间由编译器 `__DATE__` / `__TIME__` 决定，实际值随构建时刻变化；`SYSCLK=` 是运行态 `HAL_RCC_GetSysClockFreq()` 的返回值，实测 **`72000000`**（72 MHz），闭环工作流附录 B #13。
- 一行、`\r\n`（`0x0D 0x0A`）结尾、只在复位/上电后出现一次；Latin1 逐字节捕获 `LEN=149`：前导 1 字节 + 日志正文 148 字节。连续 3 次独立复位正文逐字一致、无乱码、无截断。
- **前导字节 `0xFC` 如实登记**：3 次恒定、位于 `[INF]` 之前、仅 1 字节，断点停住时尚未执行日志语句就已出现。按第 6 节失败回退表逐项核对（HSE/PLL 与运行态 72 MHz、终端编码与 115200/8N1、CRLF、日志格式串以 `[INF]` 开头、`_write` 只做透传无发送 `0xFC` 的代码），判定为**复位瞬间 RS232 线路的单个确定性毛刺**（MCU 复位期间 PA9 高阻，`U5 ST3232` 输入浮空 → TX 电平跳变被 PC 侧 UART 采为 1 个杂散字节），**非固件输出、非波特率/编码问题**。不影响验收 ⑥：日志正文无乱码无截断；终端侧忽略首字节或以 `[` 作为行起始判据即可。

---

## 5. B 条件与执行结果（**已执行完毕，⑥⑦ 达成**）

按用户指令，烧录与终端观测必须先同时满足；两个前置均已于 2026-11-12 闭环：

| # | 前置 | 状态 | 证据 |
| --- | --- | --- | --- |
| 1 | `BOOT0=0`（User Flash 启动） | ✅ **已达成** | 用户人工把 BOOT0 跳线改到 `2-3`（表 2 第一行 User Flash；BOOT1 可 ANY），断电后重新上电使 BOOT 引脚重新采样。J-Link 只读 halt 实测 `PC` 由 `0x1FFFF3B6`（System Memory）变为 **`0x08004FE6`（主 Flash 区）**，`0x00000004` 复位向量 `0x08005c69` 指向 Flash。手册 2.2 的 `JP8/JP9` ↔ `BOOT0/BOOT1` 与锁定版相反的差异**不改写锁定版映射表**，仅记录 |
| 2 | 用户显式授权烧录 | ✅ **已达成** | 用户明确回复「可以烧录」；`@docs/stage2_build_report.md` 第 222 行的「不烧录、不改跳线」约束至此被显式授权解除，旧授权不沿用 |

**烧录前只读环境复核**（不烧录、不收发串口数据）：`COM3` FTDI `VID_0403/PID_6001` `Status OK`，`COM3_OPEN_OK/COM3_CLOSED_NO_DATA`；J-Link `VID_1366&PID_0101\000000123456` `Status OK`、`PORT_2331_FREE`；`JFlash.exe`/`JLink.exe`/`JLinkGDBServerCL.exe` 均在实测目录；`build/Debug` 的 `.elf/.hex/.bin` 齐全。

**B 执行结果**（参数沿用阶段 2 实测 `-device STM32F103ZE -if SWD -speed 4000 -port 2331 -singlerun -nogui`）：

| 项 | 实测结果 | 结论 |
| --- | --- | --- |
| 烧录 | GDB `file` + `load`：`Start address 0x08000694, load size 14944`，`Comparing flash ... Done. / Erasing flash ... Done. / Programming flash ... Done.`，`SERVER_SELF_EXIT=True`、`PORT_2331_FREE` | 阶段 3 固件已写入主 Flash；未写 option bytes、未启用读保护 |
| ⑥ COM3 启动日志 | `115200/8N1/None/CRLF`，Latin1 逐字节读取；每次 `monitor reset` + `monitor go`，连续 3 次均 `LEN=149`、正文逐字一致、`SYSCLK=72000000Hz`、结尾 `0x0D 0x0A`、正文无乱码无截断（前导 `0xFC` 单独登记，见第 4 节） | ✅ **达成**；附录 B #13 运行态验证同步闭环 |
| ⑦ 断点单步 | GDB `break main.c:171` → `AT_BREAKPOINT_HIT=True`；停住时 `BEFORE_NEXT_LEN=1`、仅 `<0xFC>`，无 `[INF]` 日志；`next` 后 `AFTER_NEXT_LEN=148`、完整日志出现 | ✅ **达成**；日志与执行顺序一致，可用于阶段 5 单步定位 |

> **阶段 3 整体验收通过**（①–⑦ 全部达成）。详细逐字节证据见 `docs/stage3_report.md` 第 5 节与第 6.3 节。⑦ 用临时 GDB 命令文件完成、用完即删，阶段 5 才创建调试前端配置。

---

## 6. 失败回退表（对应工作流 429–436 行）

| 现象 | 回退动作 |
| --- | --- |
| 终端无任何输出 | 按接线表复查：`CON1` 是 **RS232 电平**必须配 RS232-USB 线（误接 USB-TTL 会无输出甚至损坏）；确认 `COM3`、共地、TX/RX 交叉、115200 8N1、无流控；确认板卡已上电、`JP12` 保持默认不安装、未改任何跳线；确认 `BOOT0=2-3` 且重新上电 |
| 乱码 | 核对 HSE=8 MHz 与 PLL×9→72 MHz（`BRR=0x271`）；确认终端 115200/8N1、编码 UTF-8/ASCII、CRLF 换行；检查 RS232-USB 线质量 |
| `printf` 无输出但不报错 | 检查链接规格：`nano.specs` / `nosys.specs` 是否生效；本工程已实测选定 `_write` 强定义（非 `__io_putchar`），并用 `setvbuf(..., _IONBF, 0)` 关闭缓冲；若仍无输出，检查 `huart1` 是否完成 `MX_USART1_UART_Init()`、PA9/PA10 是否配置正确 |
| 单个非 ASCII 前导字节（实测 `0xFC`） | 先按字节级捕获确认是复位瞬间的 1 个杂散字节而非持续乱码：核对运行态 `SYSCLK=72000000Hz`、正文 148 B 完整且 `\r\n` 结尾、3 次一致、断点停住时即出现；若均符合，判为 RS232 线路复位毛刺，终端忽略首字节或以 `[` 作行起始判据；**不要**因此改动时钟、`_write` 或接线。若正文本身出现乱码，按上两行回退 |
| 日志刷屏导致单步困难 | 降低等级（`-DDEBUG_LOG_LEVEL=2` 只保留 ERROR/WARN，或 `0` 全关），只在状态切换打印；本工程已做到「仅启动 1 行」，主循环与 ISR 均无打印 |
| 编译缺 HAL 源 / 链接报 `undefined reference` | 按 `@docs/stage2_hal_source.md` 第 4/6 节的官方来源规则补 `stm32f1xx_hal_*.c`，并同步 `Core/Inc/stm32f1xx_hal_conf.h` 模块宏；本阶段无新增外设，二者均未改动 |
| `BOOT0` 未闭环 / 未获烧录授权 | **不烧录**，回退到 A 范围（编译、静态验证、文档、报告），把 ⑥⑦ 标记为「挂起，待 BOOT0=0 人工确认 + 用户授权烧录」。本次两项前置均已满足，该回退未触发 |

---

## 7. 边界自检

**已做（均在 B 授权范围内）**：

- 烧录 Flash 一次（GDB `load`，14,944 B）；未在授权前烧录；
- COM3 连续 3 次复位捕获启动日志，Latin1 逐字节核对；
- GDB 在 `main.c:171` 设断点并 `next` 单步，比对断点前后终端内容；
- 每次 J-Link 会话 `SERVER_SELF_EXIT=True`、`PORT_2331_FREE`、无残留进程；临时 GDB 命令/标记脚本用完已删除。

**未做**：

- **AI 未改跳线**：`BOOT0` 跳线由用户人工改到 `2-3`，`JP5/JP6/JP9/JP12` 未动；未插拔 `J5`；未改驱动、未终止用户进程、未升级 J-Link FW、未使用 J-Flash、未写 option bytes / 读保护；
- 未创建 `.vscode/` / `launch.json` 或任何调试前端配置（阶段 5）；未实现蜂鸣器鸣叫/变频（阶段 4）；未复用 PA13/PA14；
- 未修改 `STM32_Buzzer_PWM_Workflow.md`、`docs/hardware_pins.md`、`docs/env_check.md`、`docs/stage2_*.md`、`scripts/env_check.ps1`、两个 PDF 与既有归档计划；`.codebuddy/` 未删除；
- **未虚报**：复位瞬间前导字节 `0xFC`、⑥ 的「正文无乱码」口径、`JP12` 现场目视 / `JP6` JTAG 档等未闭环项均如实登记。
