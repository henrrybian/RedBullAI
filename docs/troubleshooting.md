# 阶段 6：故障排查表与阶段级失败回退矩阵

> 项目：STM32F103ZET6 红牛开发板，蜂鸣器 PWM 鸣叫（路径 A），USART1 Debug，J-Link SWD
> 生成日期：2026-11-12 ｜ 平台：Windows + PowerShell ｜ 开发前端：CodeBuddy IDE ｜ 用户零手写代码
> 依据：`STM32_Buzzer_PWM_Workflow.md` 附录 C（800–817 行）与回退矩阵（712–720 行）；`@docs/debug_guide.md`（T0–T6 决策树）、`@docs/stage5_report.md` 第 3 节、`@docs/uart_wiring.md` 第 6 节、`@docs/hardware_pins.md`。
> 通用规则：每步记录 GDB Server 首行错误文本 + 当前跳线/接线状态；同一动作不盲目重试超过 2 次；烧录/下载动作须用户当轮显式授权；J-Link FW 升级弹窗一律取消。

---

## 1. 阶段级失败回退矩阵（对照工作流 712–720 行）

| 端到端步骤失败发生在（`acceptance_checklist.md` 第 1 节编号） | 回到阶段 | 需要重新确认的关键项 | 本项目锁定结论/证据出处 |
| --- | --- | --- | --- |
| 步骤 1–2（断电静态检查 / 上电） | **阶段 1** | 跳线状态（JP5 短接 / JP6=SWD / JP8 BOOT0=0 / JP9 不强拉 PB2）、串口电平（RS232 vs TTL）、蜂鸣器类型与有效电平 | `hardware_pins.md` 第 4 节 9 项复核；`uart_wiring.md` 第 2 节接线表 |
| 步骤 2 上电枚举失败（COM3 / J-Link 驱动） | **阶段 0** | USB 枚举、驱动状态（`CM_PROB_PHANTOM` = 未插入）、工具绝对路径 | `env_check.md` 10.1；`hardware_pins.md` 附录 B.1 |
| 步骤 3（编译） | **阶段 2**（产物/链接/时钟类失败）或 **阶段 5**（PATH/命令类失败） | 链接脚本、启动文件、HAL 驱动源列表、时钟配置；会话级 PATH、实测绝对路径 | `build_commands.md` 第 1–4 节；`stage2_*.md` |
| 步骤 4 / 8（烧录 / 单步调试） | **阶段 5** | J-Link 参数（`-device STM32F103ZE -if SWD -speed 4000`）、`{SVD}` 路径、Debug `-O0 -g3`、`BOOT0`、主选/兜底路径选择（主选连续 2 次失败即切兜底 `scripts/debug_gdb_cmds.txt`） | `debug_guide.md` T0–T6；`stage5_report.md` 第 2/3 节 |
| 步骤 5 / 9（日志 / 日志与调试同步） | **阶段 3** | `printf` 重定向（`_write` + `setvbuf` 无缓冲）、日志等级与频率、波特率与时钟（BRR=0x271）、终端 115200/8N1/None/CRLF | `uart_wiring.md` 第 1/3/6 节；`stage3_report.md` |
| 步骤 6–7 / 10（鸣叫 / 波形替代 / 回归） | **阶段 4** | 有效电平是否取反（`ODR` 响=高）、有源/无源分支语义、PWM 路径是否需从 A 改为 B（飞线 CN1 PB0/TIM3_CH3）、序列表与 `s_sequence[]` 一致性 | `buzzer_sequence.md` 第 1–2 节；`hardware_pins.md` 第 5 节 |

**回退后处置**：在对应阶段文档的失败回退表内逐项处置并记录证据 → 重新执行端到端清单第 1–10 步中受影响的后续步骤（前序已通过步骤不必重复，但步骤 1 静态检查在每次断电处置后必须重做）。

---

## 2. 故障排查表（现象 / 最可能原因 / 排查动作 / 对应阶段）

> 「排查动作」全部为可观测动作（命令输出、回显文本、目视结果、听感回报）；前 9 项为硬约束必覆盖现象。

### 2.1 必覆盖现象

| # | 现象 | 最可能原因 | 排查动作 | 对应阶段 |
| --- | --- | --- | --- | --- |
| 1 | 蜂鸣器完全不响 | `JP5 BEEP` 未短接；`JP9 BOOT1` 跳线强拉 PB2；供电未到位 | 断电目视 JP5/JP9 跳线帽（照片回传）；F5 停在 main 后（暂停态）读 `GPIOB->ODR`（`0x40010C0C`）bit2：响段应为 1。若 `ODR`=1 仍无声 → 依次查 JP5 → 5V 供电 → DZ1/Q1 拓扑（万用表通断档） | 1、4 |
| 2 | 蜂鸣器一直响不停 | 有效电平被取反（静音段 PB2=1）；或序列未循环到静音段 | 暂停态读 `ODR` bit2 与当前段号对照：静音段（seg=2/4/6/8）应 `0x10`；读变量 `g_buzzer_seg_index` 是否卡死不推进；对照 `buzzer_sequence.md` 第 2 节 on/off | 1、4 |
| 3 | 音调不对 | 物理限制：**有源蜂鸣器音调由内部振荡器决定，固件无法变频**（`buzzer_sequence.md` 第 1 节硬约束）；或把 `freq_hz` 误解为音调 | 核对代码语义：`freq_hz` 只表达通断节奏、无任何变频调用（附录 B #24）；确认听感差异仅来自响/停节奏而非音调；确需变调属路径 C（换无源蜂鸣器），不在本路径范围 | 1、4 |
| 4 | 日志无输出 | CON1 误接 USB-TTL（RS232↔TTL 混接）；未共地；流控开启；BOOT0≠0 未从 Flash 启动 | `Get-PnpDevice -Class Ports` 确认 COM3 存在且 `Status OK`；核对终端 115200/8N1/**流控 None**/CRLF、本地回显关闭；目视 CON1 侧是 RS232-USB 线；F5 停 main 读 PC：须 `0x080xxxxx`（`0x1FFFFxxx` = BOOT0≠0 → 断电改 2-3 重上电） | 1、3、5（T5） |
| 5 | 日志乱码 | 时钟不准导致波特率偏差；终端编码/换行配置错；线缆质量 | 启动日志 `SYSCLK` 必须为 `72000000Hz`（否则查 HSE 8 MHz / PLL×9）；`USART1 BRR` 暂停态读值应为 `0x271`；终端设 115200/8N1、UTF-8/ASCII、CRLF；Latin1 逐字节捕获确认是「正文乱码」还是「仅复位 1 个前导杂散字节 `0xFC`/`0xF8`」（后者为 RS232 复位毛刺，忽略首字节即可，**不要**改时钟/接线） | 2、3 |
| 6 | 断点不命中 | 非 `-O0 -g3` 构建；`.elf` 与板上固件不一致；断点落在无效行 | 按 `debug_guide.md` 第 3 节顺序：① `build/Debug/build.ninja` FLAGS 核对含 `-O0 -g3`；② 重新 `cmake --build build/Debug`，确认 `.elf` 时间戳晚于最后源码改动、烧录与加载符号是同一份产物；③ `arm-none-eabi-nm ... \| Select-String Buzzer_Tick` 应有 `T` 条目；④ 断点改设 `buzzer.c` 的 `Buzzer_LoadSegment(next, now);` 行（阶段 4 实测稳定命中点）；⑤ 变量 optimized out 按 `debug_guide.md` 第 4 节（`volatile` 已确认，勿改动） | 2、5 |
| 7 | J-Link 连不上 | `JP6` 非 SWD 档；Vref 异常；未共地/排线未插到位；SWD 引脚被固件复用；BOOT0 错；残留进程占端口 | 按 `debug_guide.md` T0–T6 顺序执行勿跳步：T0 查进程/端口（`Get-Process JLink*,JFlash*`、`Get-NetTCPConnection -LocalPort 2331,50000,50001,50002`）→ 用实证参数重试一次 → T1 断电查 JP6 → T2 读 `Target voltage:`（应 3.31 V）→ T3 查共地与 J5 20pin（断电插拔）→ T4 核对 PA13/PA14 无复用（静态自检应 0 命中）→ T5 读 PC 判 BOOT0 → T6 降速 4000→1000→100 重试。每次记录首行错误文本 | 2、5 |
| 8 | PWM 频率偏差大（节拍/段时长不符） | TIM2 分频配置错（应 `PSC=72-1`、`Period=100-1` → 10 kHz）；HSE/PLL 错；中断抖动 | 启动日志核对 `SYSCLK=72000000Hz`；暂停态读 `TIM2 PSC`（`0x40000028`）应 `0x47`、`ARR`（`0x4000002C`）应 `0x63`；听感/日志时序与序列表比对（±50 ms 内为 SysTick 粒度正常）；有示波器时才测 PB2 原始波形（可选） | 2、4 |
| 9 | GDB Server 首次启动 `Timeout` | 首次 USB 枚举 / 安全软件首次扫描 exe 的瞬时超时（`stage5_report.md` 3.1 已定性：非配置缺陷） | **处置按 T0 + 重试一次**：① `Get-Process JLink*,JFlash*,GDB*` 无残留；② `Get-NetTCPConnection -LocalPort 50000,50001,50002,2331` 全空闲；③ 手工启动 `JLinkGDBServerCL.exe -if swd -port 2331 -device STM32F103ZE -speed 4000` 确认 10 s 内存活并打印设置；④ **重试 F5 一次**（阶段 5 实测重试即成功）。连续 2 次失败再切兜底 `scripts/debug_gdb_cmds.txt`，不盲目改 `launch.json` | 5 |

### 2.2 其他已知现象（继承工作流附录 C）

| # | 现象 | 最可能原因 | 排查动作 | 对应阶段 |
| --- | --- | --- | --- | --- |
| 10 | 暂停态外设视图报 `readMemory failed @ 0x40000400/0x40013800` | GDB Server 在运行态拒绝读 APB 外设（扩展行为限制，非缺陷） | **断点命中（⏸）后**再展开/刷新 XPERIPHERALS；运行态该报错直接忽略（`stage5_report.md` 3.2 使用规范） | 5 |
| 11 | `printf` 无输出但不报错 | 链接规格/重定向问题 | 核对 `_write` 强定义 + `setvbuf(...,_IONBF,0)` 关缓冲 + `MX_USART1_UART_Init()` 已执行（本工程已实测选定，勿改） | 3 |
| 12 | 复位瞬间 1 个前导杂散字节（`0xFC`/`0xF8`） | 复位期间 PA9 高阻、ST3232 输入浮空 → 确定性 RS232 毛刺（非固件输出） | Latin1 逐字节捕获确认：仅 1 字节、3 次恒定、断点停住时已出现、正文 148 B 完整 → 终端忽略首字节或以 `[` 作行起始判据，**不要**改时钟/`_write`/接线 | 3 |
| 13 | 烧录 `VerifyBin` 校验不一致 | 擦写瞬时干扰 | 重试一次；仍失败换 `.hex` 口径单独烧录再校验；确认未启用读保护（**严禁写 option bytes / 解锁**，报告用户评估） | 5 |
| 14 | J-Link 报固件升级弹窗 | V8 兼容/克隆类触发升级提示 | **一律取消，严禁升级**（盲刷有变砖风险）；截图登记后继续 | 0、5 |
| 15 | 编译报 HAL 函数未定义 | HAL 驱动源未加入构建/版本不匹配 | 按 `stage2_hal_source.md` 第 4/6 节官方来源规则补 `stm32f1xx_hal_*.c` 并同步 `stm32f1xx_hal_conf.h` 模块宏 | 2 |
| 16 | 兜底 GDB 序列报错 | PATH/端口/参数不匹配 | 逐条重跑回传完整错误文本；确认 Server 与 `target remote` 端口一致（2331）；`arm-none-eabi-gdb` 用实测绝对路径；`-singlerun` 下 GDB 断开后 Server 自动退出属预期 | 5 |

**处置纪律**（继承 `debug_guide.md`）：同现象排查动作最多执行 2 轮；2 轮未解决 → 按第 1 节矩阵回退到对应阶段，把首行错误文本 + 跳线/接线状态 + 已执行动作清单回传，重新定位后继续；主选调试路径连续 2 次失败才切兜底。
