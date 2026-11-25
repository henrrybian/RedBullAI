---
name: 阶段3-串口Debug输出
overview: 执行《STM32 蜂鸣器 PWM 全 AI 生成工作流（CodeBuddy IDE 版）》阶段 3：先实测链接规格决定 printf 重定向方式，生成串口日志模块并接入构建，跑通 Debug/Release 编译与静态自检，产出接线文档与阶段报告；在 BOOT0=0 实测闭环且用户明确授权烧录后，完成 Flash 烧录、COM3 连续 3 次启动日志观测与 GDB 断点单步，验收 ①–⑦ 全部达成，阶段 3 整体验收通过。
status: completed
completed_at: "2026-11-12"
todos:
  - id: read-stage3-context
    content: 实际读取全部 @ 上下文与阶段 3 规范/边界
    status: completed
  - id: probe-link-specs
    content: 探测 newlib/nosys 链接规格与 _write 原型，选定重定向方案
    status: completed
  - id: generate-debug-log
    content: 生成 debug_uart.c 与 debug_log.h 完整文件
    status: completed
  - id: integrate-main-and-cmake
    content: 增补 main.c 启动日志与 CMakeLists.txt 源列表/等级开关
    status: completed
  - id: generate-wiring-doc
    content: 生成 docs/uart_wiring.md 终端配置与接线对照表
    status: completed
  - id: build-and-static-verify
    content: 跑 cmake 配置/Debug 构建/size/Release 等级开关与静态自检
    status: completed
  - id: archive-stage3
    content: 生成 docs/stage3_report.md 并归档本计划供阶段 4 引用
    status: completed
  - id: stage3-b-burn-and-observe
    content: 用户授权后烧录 Flash、COM3 连续 3 次读日志、GDB 断点单步验证
    status: completed
  - id: stage3-b-archive
    content: 把 B 实测结果回写 stage3_report.md / uart_wiring.md / 本计划
    status: completed
---

## 用户要求与阶段门

- 只执行阶段 3（工作流 372–438 行），阶段 4（440 行起）业务与阶段 5 调试前端配置一律不越界。
- A（默认）：串口重定向与日志能力、构建接入、编译与静态验证、接线文档、阶段报告。
- B（条件）：**必须先由用户显式授权烧录 + 人工确认 `BOOT0=0` 闭环**，才允许烧录与 COM3 终端观测、断点单步。
- **B 已于 2026-11-12 执行完毕**：用户把 BOOT0 改到 `2-3` 并重新上电，J-Link 只读 halt 实测 `PC=0x08004FE6` 闭环前置①；用户明确回复「可以烧录」闭环前置②。随后烧录 Flash 一次、COM3 连续 3 次读到一致启动日志、`main.c:171` 断点 + `next` 单步验证通过。**验收 ①–⑦ 全部达成，阶段 3 整体验收通过。**

## 关键决策（均有实测证据）

1. **重定向选 `_write`，不选 `__io_putchar`**：`cmake/gcc-arm-none-eabi.cmake:20` 已用 `-specs=nano.specs -specs=nosys.specs`；实测 `libg_nano.a:libc_a-writer.o` 为 `U _write`，`libnosys.a:write.o` 单独提供 `T _write`，且 libnosys 中 `__io_putchar` 命中数 = 0。强定义 `_write` 后链接器不再抽取 `write.o`，无 undefined reference / 符号重复；`nosys.specs` 保留，**工具链文件未改动**。
2. **原型用 `_READ_WRITE_RETURN_TYPE`**（`sys/unistd.h:277`，`sys/config.h:83` 定义为 `_ssize_t`），避免 conflicting types。
3. **`setvbuf(stdout/stderr, NULL, _IONBF, 0)`**：裸机 `_isatty` 返回 0 时 stdout 默认全缓冲，显式无缓冲保证单行日志即时发送、单步顺序一致；⑦ 断点单步实测验证。
4. **编译期等级开关**：`CMakeLists.txt` 新增 `DEBUG_LOG_LEVEL` 缓存变量，Debug=4 / Release=2 / 可 `-DDEBUG_LOG_LEVEL=0` 全关。
5. **未新增 HAL 模块**：`HAL_UART_MODULE_ENABLED` 与 `stm32f1xx_hal_uart.c` 阶段 2 已就位，`stm32f1xx_hal_conf.h` 未改动。

## 交付物

| 文件 | 动作 |
| --- | --- |
| `Core/Inc/debug_log.h` | 新增完整文件：四级日志宏、等级开关、固件版本/MCU 常量、初始化声明 |
| `Core/Src/debug_uart.c` | 新增完整文件：`_write` + `HAL_UART_Transmit` 阻塞发送 + 关闭缓冲 |
| `Core/Src/main.c` | 定点增补：时钟快照之后一行启动日志（含运行态 `g_sysclk_hz`），主循环仍空转 |
| `CMakeLists.txt` | 定点增补：`APP_SOURCES` 加入 `debug_uart.c`，`DEBUG_LOG_LEVEL` 编译定义 |
| `docs/uart_wiring.md` | 新增完整文件：终端配置清单、三种接线对照表、电平/共地硬约束、**实测启动日志与 B 执行结果**、失败回退 |
| `docs/stage3_report.md` | 新增完整文件：验收结果、**B 烧录/终端/断点逐字节证据**、L1–L9 更新、阶段 4 交接 |
| 本计划 | 新增归档，供阶段 4 `@` 引用；既有归档计划未改动，`.codebuddy/` 未删除 |

## 验收证据摘要

- Debug：`CONFIGURE_EXIT=0`，构建 **20/20**、`BUILD_EXIT=0`；`.elf/.hex/.bin/.map` 齐全；`text 14832 / data 112 / bss 2072` → Flash 14,944 B、RAM 2,184 B（512K/64K 内）。
- 链接：`_write is not implemented` 警告消失；map 实测 `.text._write ... Core/Src/debug_uart.c.obj`，`libnosys.a(write.o)` 出现 0 次；`nm` 有 `T _write` / `T printf` / `T setvbuf`。
- 等级开关：Debug `-DDEBUG_LOG_LEVEL=4`（`.bin` 含 `STAGE3`/`[INF]`）；Release `-DDEBUG_LOG_LEVEL=2`（`.bin` 中 `STAGE3`/`[INF]`/`[DBG]` 均为 False）。
- 静态自检：ISR 内无 `printf`（仅注释）、全工程 `LOG_*` 调用点 1 处、主循环空转、PA13/PA14/remap/DBGMCU 命中全为注释、`-O0 -g3` 保留、`launch.json` 0 个、`read_lints` 0 诊断。
- B ⑥：烧录 `load size 14944`，`Comparing/Erasing/Programming ... Done.`；COM3 `115200/8N1` 连续 3 次 `LEN=149`、正文逐字一致、`SYSCLK=72000000Hz`、结尾 `0x0D 0x0A`，正文无乱码无截断；前导 1 字节 `0xFC` 判为复位瞬间 RS232 线路毛刺，如实登记。
- B ⑦：`Breakpoint 1, main () at ... main.c:171`；停住时 `BEFORE_NEXT_LEN=1`（仅 `0xFC`），`next` 后 `AFTER_NEXT_LEN=148` 完整日志出现。
- 验收：**①–⑦ 全部已达成（含 ⑥⑦），阶段 3 整体验收通过**；附录 B #13（运行态 72 MHz）与 L1、L8 同步闭环。

## 阶段 4 交接

- 可 `@` 引用：`docs/hardware_pins.md`（锁定版唯一真相源）、`docs/uart_wiring.md`、`docs/stage3_report.md`、`docs/clock_tree.md`、`docs/init_selfcheck.md`、`docs/stage2_build_report.md`、`docs/stage2_hal_source.md`、`docs/stage2_manual_notes.md`、本计划，以及 `Core/Inc/debug_log.h`、`Core/Src/debug_uart.c`、`Core/Src/main.c`、`CMakeLists.txt`。
- 锁定事实：有源蜂鸣器、高电平有效、`{PWM_PATH}=A`、PB2(pin48) 无 TIM 通道；工作流 273 行禁止为有源蜂鸣器生成变频改变音调的逻辑；阶段 4 才实现鸣叫/节奏业务，非阻塞状态机。
- 继续禁止：ISR 内 `printf`、高频刷屏、复用 PA13/PA14、SWJ remap/DBGMCU 关闭；Debug 固定 `-O0 -g3`；不创建调试前端配置（阶段 5）。
- 可直接复用：`printf`/`LOG_*`/四级等级开关、四个运行态频率快照（实测 72 MHz）、`g_tim2_update_count`、Debug+Release 两套构建，以及已验证的烧录/复位/断点链路 `-device STM32F103ZE -if SWD -speed 4000 -port 2331 -singlerun -nogui` + GDB `file/load/monitor reset/break/next`。
- **当前板卡状态**：`BOOT0=2-3`（User Flash），Flash 内为阶段 3 固件；`JP6` 仍为 JTAG 档但 SWD 实测可用；J-Link 为 V8 兼容/克隆类，不做 FW 升级。阶段 4 若需**再次烧录**，仍须取得当轮用户显式授权（本次授权不沿用）。
- 未闭环但不阻塞：L2 `JP8/JP9` 编号差异、L3 `JP6`、L5 `JP12` 现场目视、L6/L7 人工硬件复核、L9 CPUID/SVD（阶段 5）。

## 本阶段已做与未做的事

**已做（B 授权范围内）**：烧录 Flash 一次（GDB `load`）、COM3 连续 3 次复位捕获并逐字节核对日志、GDB `main.c:171` 断点 + `next` 单步比对；每次会话 `SERVER_SELF_EXIT=True`、`PORT_2331_FREE`，临时脚本用完即删。

**未做**：AI 未改跳线（BOOT0 由用户人工改动）、未改驱动、未终止用户进程、未升级 J-Link FW、未使用 J-Flash、未写 option bytes / 读保护、未创建调试前端配置、未实现阶段 4 蜂鸣器业务、未修改工作流文档与锁定版/阶段 2 文档/PDF/脚本/既有归档计划、未虚报任何终端实测结果（`0xFC` 前导毛刺如实登记）。

---

## 补充记录（2026-11-12，阶段 4 引用时以此为准）

用户提供了手册「表 2 启动方式选择跳线设置描述」，并更正硬件状态：

1. 表 2 原文：`BOOT1(JP8)` / `BOOT0(JP9)`；`BOOT0 2-3` + `BOOT1 ANY` = **User Flash**；`BOOT0 1-2` + `BOOT1 2-3` = **System Memory**；`BOOT0 1-2` + `BOOT1 1-2` = Embedded SRAM。与锁定版 `JP8 BOOT0 / JP9 BOOT1` 编号相反，**不改写锁定版**。
2. 用户确认 `CON1` 侧为 **RS232 电平**、板子已上电、J-Link 连接稳定；随后把 BOOT0 改到 `2-3` 并重新上电。
3. **只读实测闭环**：`-device STM32F103ZE -if SWD -speed 4000 -port 2331` + GDB（未 `file`、未 `load`、未写寄存器、未复位）→ 改跳线前 `PC=0x1FFFF3B6`（System Memory），改后 **`PC=0x08004FE6`（主 Flash）**，`0x00000004=0x08005c69`，`Target voltage: 3.31 V`；两次 `SERVER_SELF_EXIT=True`、`PORT_2331_FREE`、无残留进程。
4. 只读环境复核：`COM3` FTDI `VID_0403/PID_6001` `Status OK`、`COM3_OPEN_OK/CLOSED_NO_DATA`；J-Link `VID_1366&PID_0101\000000123456` `Status OK`；`JFlash.exe`/`JLink.exe`/`JLinkGDBServerCL.exe` 在实测目录；`build/Debug` 的 `.elf/.hex/.bin` 齐全。
5. 上述只读阶段曾把 ⑥⑦ 记为「挂起、待授权」；该状态已被用户「可以烧录」及下列 B 执行结果取代，以本节后续内容为准。

## B 执行记录（2026-11-12，用户授权后）

- **前置**：① `BOOT0=0` 由只读 halt 实测闭环；② 用户明确回复「可以烧录」，阶段 2 交接第 222 行「不烧录、不改跳线」约束被显式解除，旧授权不沿用。
- **烧录**：GDB `file` + `load`，`Start address 0x08000694, load size 14944`，`Comparing flash ... Done. / Erasing flash ... Done. / Programming flash ... Done.`，`SERVER_SELF_EXIT=True`、`PORT_2331_FREE`；未写 option bytes、未启用读保护。
- **⑥ 终端日志**：COM3 `115200/8N1/None/CRLF`，Latin1 逐字节读取；每次 `monitor reset` + `monitor go`，3 次均 `LEN=149`、正文逐字一致、`SYSCLK=72000000Hz`、结尾 `0x0D 0x0A`：
  `[INF] STAGE3 FW=0.3.0 BUILD=Nov 12 2026 13:54:26 MCU=STM32F103ZET6 SYSCLK=72000000Hz UART=USART1/115200-8N1 BEEP=PB2(pin48)/active-high PWM_PATH=A\r\n`
  `SYSCLK` 来自运行态 `HAL_RCC_GetSysClockFreq()`，闭环工作流附录 B #13。每次另有 1 个前导字节 `0xFC`，判定为复位瞬间 RS232 线路毛刺（非固件输出、非波特率/编码问题），⑥ 的「无乱码」限定为日志正文。
- **⑦ 断点单步**：GDB `-batch -x` 临时命令文件在 `main.c:171` 设断点 → `AT_BREAKPOINT_HIT=True`，`BEFORE_NEXT_LEN=1`（仅 `<0xFC>`、无 `[INF]`），`next` 后 `AFTER_NEXT_LEN=148` 完整日志出现；未创建 `launch.json`，临时脚本已删除。
- **结论**：验收 ①–⑦ 全部达成，**阶段 3 整体验收通过**。完整证据见 `docs/stage3_report.md` 第 5 / 6.3 / 7 节与 `docs/uart_wiring.md` 第 4 / 5 节。
