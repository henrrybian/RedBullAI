# 阶段 3 报告：串口 Debug 输出

> 项目：STM32F103ZET6 红牛开发板，USART1 Debug 输出，J-Link SWD 单步调试
> 完成日期：2026-11-12 ｜ 平台：Windows + PowerShell ｜ 开发前端：CodeBuddy IDE
> 只读输入：`@STM32_Buzzer_PWM_Workflow.md` 阶段 3（372–438 行，阶段 4 自 440 行起未越界）、`@docs/hardware_pins.md`（锁定版唯一硬件真相源）、`@docs/env_check.md`、`@docs/stage2_build_report.md`（第 5 节 L1–L9、第 6 节交接第 222 行）、`@docs/stage2_manual_notes.md`、`@docs/clock_tree.md`、`@docs/init_selfcheck.md`、`@docs/stage2_hal_source.md`、`@.codebuddy/plans/阶段2-HAL初始化与工程骨架生成_d8bd792e.md`、`@Core/Src/main.c`、`@Core/Inc/main.h`、`@Core/Src/stm32f1xx_it.c`、`@Core/Inc/stm32f1xx_hal_conf.h`、`@CMakeLists.txt`、`@cmake/gcc-arm-none-eabi.cmake`
> **A/B 边界**：A（默认，零硬件操作）已全部完成；B 的两个前置（`BOOT0=0` 实测闭环 + 用户于 2026-11-12 明确回复「可以烧录」）均已满足，**B 已执行完毕**：固件已烧录、COM3 终端连续 3 次读到一致启动日志、GDB 断点单步验证通过。B 结论见第 5 节与第 6.3 节。

---

## 0. 结论摘要

| 范围 | 结论 |
| --- | --- |
| A：串口重定向 / 日志能力 / 构建接入 / 编译与静态验证 / 接线文档 / 报告 | ✅ **已达成**（验收 ①–⑤） |
| B 前置①：`BOOT0=0` | ✅ **已达成**（实测：halt PC 由 `0x1FFFF3B6` → `0x08004FE6`，复位向量 `0x08005c69` 指向 Flash；见第 6.1 节） |
| B 前置②：用户显式授权烧录 | ✅ **已达成**（用户回复「可以烧录」） |
| B⑥：烧录 + COM3 启动日志连续 3 次一致 + `SYSCLK=72000000` | ✅ **已达成**（正文无乱码无截断；另有 1 个复位瞬间前导字节 `0xFC` 如实登记，见 6.3 节） |
| B⑦：日志语句设断点，单步后终端才出现下一行 | ✅ **已达成**（`main.c:171` 断点：停住时 `LEN=1` 仅毛刺，`next` 后 `LEN=148` 完整日志） |
| 硬件动作 | 仅烧录 Flash 一次（用户授权）；AI 未改跳线（`BOOT0` 由用户人工改动）、未改驱动、未终止用户进程、未升级 J-Link FW、未写 option bytes、未启用读保护 |
| 阶段 4 业务 / 阶段 5 调试前端 | 未实现 / 未创建（未越界） |

---

## 1. 链接规格探测与重定向决策（步骤 2 实测）

| # | 探测项 | 可观测结果 | 结论 |
| --- | --- | --- | --- |
| 1 | `cmake/gcc-arm-none-eabi.cmake` 现状 | 第 20 行 `-specs=nano.specs -specs=nosys.specs -Wl,--gc-sections`；阶段 3 改动前基线重建 `BASELINE_BUILD_EXIT=0`（18/18），链接警告含 `_write is not implemented and will always fail` | newlib-nano + nosys 桩已生效，**无需改动工具链文件** |
| 2 | newlib-nano | `libg_nano.a:libc_a-writer.o` → `U _write` / `T _write_r`；`sys/unistd.h:277` = `_READ_WRITE_RETURN_TYPE _write (int, const void *, size_t)`；`sys/config.h:83` 把该宏定义为 `_ssize_t` | 必须提供 `_write`；用 `_READ_WRITE_RETURN_TYPE` 宏保证原型一致，避免 conflicting types |
| 3 | libnosys | `libnosys.a:write.o` 单独提供 `T _write`（并带 `__evoke_link_warning__write`）；对 `__io_putchar` 的符号命中数 = **0** | 在 `nosys.specs` 下 libnosys 的 `_write` **不回调** `__io_putchar`，单实现 `__io_putchar` 不会让 `printf` 输出 |
| 4 | 官方 API 核对 | `stm32f1xx_hal_uart.h:749`：`HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef*, const uint8_t*, uint16_t, uint32_t)`；`stm32f1xx_hal_def.h:40/56`：`HAL_OK`、`HAL_MAX_DELAY` | 未臆造 API；用 100 ms 有界超时而非无限等待 |
| 5 | 缓冲行为 | 裸机下 nosys 的 `_isatty` 返回 0，stdout 默认全缓冲；`stdio.h:201` 提供 `setvbuf`、`stdio.h:122` 定义 `_IONBF` | 在 `debug_uart_init()` 中 `setvbuf(stdout/stderr, NULL, _IONBF, 0)` 关闭缓冲，保证日志即时发送、单步顺序一致 |

**决策**：`Core/Src/debug_uart.c` 强定义 `_write`（阻塞式 `HAL_UART_Transmit(&huart1, …)`）+ `debug_uart_init()` 关闭缓冲。强定义使链接器不再抽取 libnosys 的 `write.o`，故无 `undefined reference`、无符号重复；`nosys.specs` 仍为必需（`_close/_fstat/_isatty/_lseek/_read/_exit/_sbrk` 桩），**工具链文件保持原样，未增改任何链接选项**。

**验证闭环证据**：构建后链接警告中 `_write is not implemented` 已消失，只剩 `_close/_fstat/_isatty/_lseek/_read` 桩警告；`build/Debug/RedBull_Buzzer_PWM.map` 实测 `.text._write 0x0800058c 0x4c CMakeFiles/.../Core/Src/debug_uart.c.obj`，且 `libnosys.a(write.o)` 在 map 中出现次数 = **0**；`arm-none-eabi-nm` 实测 `0800058c T _write`、`08002cac T _write_r`、`08000560 T debug_uart_init`、`08002a20 T printf`、`08002a44 T setvbuf`。

**HAL 模块**：本阶段未新增外设，`HAL_UART_MODULE_ENABLED` 与 `stm32f1xx_hal_uart.c` 在阶段 2 已就位（14 个官方源不变），因此按 `@docs/stage2_hal_source.md` 第 4/6 节规则**无需补官方源、无需改 `Core/Inc/stm32f1xx_hal_conf.h`**。

---

## 2. 本阶段文件清单（全部由 AI 以完整文件生成，用户零手写代码）

| 文件 | 动作 | 内容 |
| --- | --- | --- |
| `Core/Inc/debug_log.h` | **新增完整文件** | `ERROR/WARN/INFO/DEBUG` 四级宏、编译期等级开关、`FIRMWARE_VERSION=0.3.0`、`FIRMWARE_MCU_NAME`、单行 `\r\n` 输出、`debug_uart_init()` 声明；头文件内写明「禁止 ISR 内 printf / 禁止刷屏」 |
| `Core/Src/debug_uart.c` | **新增完整文件** | `_write` 强定义 + `HAL_UART_Transmit` 阻塞发送（100 ms 超时）+ `setvbuf(..., _IONBF, 0)`；文件头逐项记录链接规格实测依据 |
| `Core/Src/main.c` | **定点增补** | 增加 `#include "debug_log.h"`；在四个频率快照之后调用 `debug_uart_init()` 与唯一一行 `LOG_INFO(...)`（固件版本 / `__DATE__ __TIME__` / MCU / `g_sysclk_hz` / `USART1/115200-8N1` / `PB2(pin48)` 与 `PWM_PATH=A`）；主循环仍空转 |
| `CMakeLists.txt` | **定点增补** | `APP_SOURCES` 由 3 个 `.c` → 4 个（加入 `Core/Src/debug_uart.c`）；新增 `DEBUG_LOG_LEVEL` 缓存变量（Debug=4 / Release=2，可 `-DDEBUG_LOG_LEVEL=N` 覆盖）并注入 `target_compile_definitions` |
| `docs/uart_wiring.md` | **新增完整文件** | 12 项终端配置清单、三种接线对照表、RS232/TTL 不可混接与共地硬约束、实测日志格式、B 前置与执行结果、失败回退、边界自检 |
| `docs/stage3_report.md` | **新增完整文件** | 本报告 |
| `.codebuddy/plans/阶段3-串口Debug输出_7c1e5a93.md` | **新增归档计划** | 供阶段 4 `@` 引用；未改动任何既有归档计划，`.codebuddy/` 未删除 |

**未改动**：`STM32_Buzzer_PWM_Workflow.md`、`docs/hardware_pins.md`、`docs/env_check.md`、`docs/stage2_*.md`、`scripts/env_check.ps1`、两个 PDF、既有归档计划、`cmake/gcc-arm-none-eabi.cmake`、`Core/Inc/stm32f1xx_hal_conf.h`、`Core/Src/stm32f1xx_it.c`、`Core/Inc/main.h`、`startup_stm32f103xe.s`、`STM32F103ZETx_FLASH.ld`、`Drivers/**`。

---

## 3. 构建与产物证据（步骤 7 实测）

### 3.1 命令（PowerShell，工具路径取自 `@docs/env_check.md` 8.3 实测）

```powershell
# 会话级补齐 cmake / ninja 的 PATH（不改机器级/用户级环境变量）
$env:PATH = 'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\cmake_x64\cmake-4.5.0-rc3-windows-x86_64\bin;' +
            'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\ninja\bin;' + $env:PATH
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/Debug
$elf = (Get-ChildItem -LiteralPath 'build/Debug' -Filter '*.elf').FullName
& 'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-size.exe' --format=berkeley $elf
```

> 沿用阶段 2 报告 3.4 节的 PowerShell 兼容写法：原生程序不会展开 `*.elf` 通配符，故用 `Get-ChildItem` 取绝对路径。

### 3.2 实测输出

| 项 | Debug | Release（等级开关验证） |
| --- | --- | --- |
| `cmake` 配置 | `Configuring done (0.1s)` / `Generating done (0.1s)`，`CONFIGURE_EXIT=0` | `CONFIGURE_EXIT=0` |
| 构建 | **20/20**，`BUILD_EXIT=0` | **20/20**，`BUILD_EXIT=0` |
| 编译定义 | `-DDEBUG_LOG_LEVEL=4 -DSTM32F103xE -DUSE_HAL_DRIVER` | `-DDEBUG_LOG_LEVEL=2 -DSTM32F103xE -DUSE_HAL_DRIVER` |
| 编译标志 | `-mcpu=cortex-m3 -mthumb -mfloat-abi=soft -O0 -g3 -Wall -Wextra -ffunction-sections -fdata-sections` | `-Os -DNDEBUG`（其余相同） |
| `size` | `text 14832 / data 112 / bss 2072`，**Flash 14,944 B**（512 KB 的 2.85%）、**RAM 2,184 B**（64 KB 的 3.33%） | `text 8032 / data 112 / bss 2072` |
| 链接 | 0 error；`_write` 桩警告消失；剩余 `_close/_fstat/_isatty/_lseek/_read` 桩警告与官方 ld 模板的 `LOAD segment with RWX permissions` 警告（均非 error，阶段 2 已记录同类现象） | 0 error，同类非 error 警告 |

**产物（`build/Debug/`，字节数实测）**：`RedBull_Buzzer_PWM.elf` 898,800；`.hex` 42,120；`.bin` 14,944；`.map` 488,194。四件齐全，容量均在 512K Flash / 64K RAM 内。

### 3.3 日志等级开关生效（编译期）

| 检查 | 结果 |
| --- | --- |
| Debug `.bin` 含 `STAGE3` / `[INF]` | `True` / `True` |
| Release `.bin` 含 `STAGE3` / `[INF]` / `[DBG]` | `False` / `False` / `False` |
| 结论 | Debug 开启 INFO/DEBUG（等级 4）；Release 等级 2 时 INFO/DEBUG 及其字符串被完全编译掉；`cmake -DDEBUG_LOG_LEVEL=0` 可全关 |

---

## 4. 静态自检（硬约束核对）

| # | 检查项 | 可观测证据 | 结论 |
| --- | --- | --- | --- |
| 1 | 无中断上下文 `printf` | `Core/Src/stm32f1xx_it.c` 中 `printf\|LOG_*` 命中 1 处，仅为第 5 行注释「不调用 printf」；`TIM2_IRQHandler` / `HAL_TIM_PeriodElapsedCallback` / `SysTick_Handler` / `USART1_IRQHandler` 均无日志调用 | ✅ |
| 2 | 无高频刷屏 | 全工程 `LOG_*` 调用点仅 1 处（`main.c:171`），位于时钟快照之后；`main.c:176-179` 主循环空转；无 `HAL_Delay`、无周期性打印 | ✅ |
| 3 | 启动日志位置与阻塞性 | `main.c:162-165` 四个频率快照 → `main.c:170` `debug_uart_init()` → `main.c:171-174` 唯一一行 `LOG_INFO`；`SYSCLK` 取 `g_sysclk_hz`；之后立即进入空循环，主循环不调用日志/不阻塞等待 | ✅ |
| 4 | SWD 引脚与调试口 | `Core/**` 中 `GPIO_PIN_13\|GPIO_PIN_14\|__HAL_AFIO_REMAP_SWJ\|DBGMCU\|HAL_GPIO_WritePin\|HAL_GPIO_TogglePin` 命中 4 处，**全部为注释**；无代码层复用/关闭；`MX_GPIO_Init` 仍只配置 PA9/PA10 | ✅ |
| 5 | Debug 可调试 | `build/Debug/build.ninja` 实测 `FLAGS = ... -O0 -g3 ...`；`read_lints` 对新增/修改文件返回 `totalCount=0`；B 阶段 GDB 能命中断点 `main.c:171` 并单步，符号可用 | ✅ |
| 6 | 未创建调试前端配置 | 工作区递归搜索 `launch.json` = 0 个文件；无 `.vscode/`；⑦ 用临时 GDB 命令文件完成，用完即删 | ✅（阶段 5 才创建） |
| 7 | 重定向无冲突 | 见第 1 节 map / nm / 链接警告证据；运行态 `printf` 实测有输出 | ✅ |
| 8 | 阶段边界 | 无 PB2 驱动、无蜂鸣器鸣叫/变频逻辑；启动日志中的 `PWM_PATH=A` 仅为锁定事实摘要 | ✅ 未越界到阶段 4 |

### 4.1 工作流附录 B #13（`SystemCoreClock = 72 MHz`）

- ✅ **已闭环（2026-11-12 运行态实测）**：烧录后 COM3 连续 3 次读到 `[INF] ... SYSCLK=72000000Hz ...`，且该值来自运行态 `g_sysclk_hz = HAL_RCC_GetSysClockFreq()`，不是代码里的常量。HSE 8 MHz → PLL×9 → 72 MHz 的时钟配置未改动，波特率正确、日志无乱码也反向印证了时钟。
- `SystemCoreClock` 与 `g_*` 频率快照仍可由 GDB / SVD 观察（`@docs/init_selfcheck.md` 第 1 项）；本项由阶段 2 的「⚠️ 待运行态验证」正式转为「✅ 已达成」。

---

## 5. 验收结果对照（全部「已达成」）

| # | 验收项 | 状态 | 证据 / 说明 |
| --- | --- | --- | --- |
| ① | cmake 配置/构建 0 error，`.elf/.hex/.bin` 齐全，size 在 512K/64K 内 | ✅ **已达成** | 第 3 节：`CONFIGURE_EXIT=0`、`BUILD_EXIT=0`、20/20、四产物齐全、Flash 14,944 B / RAM 2,184 B |
| ② | 重定向链接无冲突；启动日志在时钟快照之后、主循环不阻塞 | ✅ **已达成** | 第 1 节 map/nm、第 4 节 #3；`main.c:162-179` |
| ③ | Debug 开启 INFO/DEBUG、Release 可关闭 | ✅ **已达成** | 第 3.3 节：Debug `-DDEBUG_LOG_LEVEL=4` / Release `=2`，二进制字符串对比 |
| ④ | 无中断内 printf、无高频刷屏、SWD 未占用、`-O0 -g3` 生效 | ✅ **已达成** | 第 4 节 #1/#2/#4/#5 |
| ⑤ | `docs/uart_wiring.md` 与 `docs/stage3_report.md` 齐备且区分「已达成 / 挂起」 | ✅ **已达成** | 两份完整文件；B 执行前明确标记挂起，执行后按实测改写 |
| ⑥ | COM3 以 115200/8N1 打开，复位后稳定看到启动日志且 `SYSCLK=72 MHz`，连续 3 次一致、无乱码无截断 | ✅ **已达成** | 3 次独立复位均 `LEN=149`、正文逐字一致、`SYSCLK=72000000Hz`、结尾 `0x0D 0x0A`、无截断；见 6.3 节。**附记录项**：每次有 1 个复位瞬间前导字节 `0xFC`，判定为 RS232 线路毛刺而非日志乱码，已如实登记，不宣称「零杂散字节」 |
| ⑦ | 在日志输出语句设断点（图形化 F10 或 GDB `break`/`next`），单步后终端才出现下一行 | ✅ **已达成** | `Breakpoint 1, main () at ... main.c:171`；停住时 COM3 `LEN=1`（仅 `0xFC`）无日志，`next` 后 `LEN=148` 完整日志出现；见 6.3 节 |

> **阶段门判定**：A 范围 ①–⑤ 全部达成；B 前置①②均闭环，⑥⑦ 已由烧录 + 终端 + 断点单步实测达成。**阶段 3 整体验收通过。**
> 口径说明：⑥ 的「无乱码」针对启动日志正文；复位瞬间 1 个线路毛刺字节单独登记并按失败回退表核对完毕，不作为虚报项隐藏。

---

## 6. B 条件（烧录 / 终端观测）与执行证据

### 6.0 前置条件（两个均已满足）

| # | 前置 | 状态 | 证据 |
| --- | --- | --- | --- |
| 1 | `BOOT0=0`（User Flash 启动） | ✅ **已达成**（2026-11-12） | 用户人工把 BOOT0 跳线改到 `2-3`；J-Link 只读 halt 实测 `PC=0x08004FE6`、`0x00000004=0x08005c69`，见 6.1 节 |
| 2 | 用户**显式授权烧录** | ✅ **已达成**（2026-11-12） | 用户明确回复「可以烧录」；`@docs/stage2_build_report.md` 第 222 行的「不烧录、不改跳线」约束至此被显式授权解除，旧授权不沿用 |

### 6.1 `BOOT0=0` 实测闭环证据（2026-11-12，只读）

| 项 | 改跳线前（BOOT0 1-2） | 改跳线后（BOOT0 2-3） |
| --- | --- | --- |
| J-Link halt PC | `0x1FFFF3B6`（System Memory / bootloader） | **`0x08004FE6`（主 Flash 区）** |
| `x/1xw 0x00000004` 复位向量 | `0x08005c69` | `0x08005c69`（指向 Flash） |
| 连接回显 | `Target voltage: 3.31 V`、`Target halted` | 同左，`Target halted (PC = 0x08004FE6)` |
| 收尾 | `SERVER_SELF_EXIT=True`、`PORT_2331_FREE` | 同左，无残留进程 |

- 参数全部沿用阶段 2 实测：`-device STM32F103ZE -if SWD -speed 4000 -port 2331 -singlerun -nogui`；该阶段 GDB **未 `file`、未 `load`、未写寄存器、未复位目标**，仅 `p/x $pc` 与 `x/1xw 0x00000004`。
- 结论：BOOT 引脚在复位瞬间采样，改跳线并重新上电后内核已从主 Flash 运行 → **`BOOT0=0` 闭环**。L1/L2 中「当前是否从 Flash 启动」的疑问已消除；手册与原理图的 `JP8/JP9` 编号相反仍按硬约束**不改写锁定版**，仅记录差异。
- 当时 `0x08004FE6` 是 Flash 内**既有旧固件**的运行位置；本工程 `.elf` 在 6.3 节才烧录。

### 6.2 烧录前只读环境复核

| 项 | 实测 |
| --- | --- |
| J-Link 相关进程 / 2331 端口 | `NO_JLINK_PROCESS` / `PORT_2331_FREE` |
| 串口枚举 | `USB Serial Port (COM3)` FTDI `VID_0403/PID_6001`，`Status OK` |
| J-Link USB | `VID_1366&PID_0101\000000123456`，`Status OK`（另 2 条历史幻影） |
| COM3 可打开性 | `COM3_OPEN_OK` / `COM3_CLOSED_NO_DATA`（仅 Open/Close，未收发） |
| 烧录工具（只读列举，未执行 J-Flash） | `JFlash.exe` / `JLink.exe` / `JLinkGDBServerCL.exe` 均在 `C:\Program Files\SEGGER\JLink_V818\` |
| 待烧录产物 | `build/Debug/RedBull_Buzzer_PWM.elf/.hex/.bin` 齐全 |

**其他风险（L3/L4）**：`JP6` 用户此前确认为 JTAG 模式（切换须先断电且人工执行），但本阶段 `-if SWD -speed 4000` 已稳定连接、烧录、halt，链路可用；J-Link 为 V8 兼容/克隆类（`S/N 99999994`），**FW 升级不执行**。

### 6.3 B 执行证据（2026-11-12，用户授权后）

**烧录**（沿用阶段 2 已实测参数 `-device STM32F103ZE -if SWD -speed 4000 -port 2331 -singlerun -nogui`，GDB `file` + `load`，未臆造任何 J-Flash CLI 参数）：

```text
Loading section .isr_vector, size 0x1e4  lma 0x8000000
Loading section .text,        size 0x370c lma 0x80001e4
Loading section .rodata,      size 0x100  lma 0x80038f0
Loading section .init_array,  size 0x4    lma 0x80039f0
Loading section .fini_array,  size 0x4    lma 0x80039f4
Loading section .data,        size 0x68   lma 0x80039f8
Start address 0x08000694, load size 14944
Transfer rate: 364 KB/sec, 2490 bytes/write.
Downloading 484 bytes @ address 0x08000000 / 14092 bytes @ 0x080001E4 / ...
Comparing flash [....] Done.  Erasing flash [....] Done.  Programming flash [....] Done.
Writing register 'pc' = 0x08000694     Shutting down...
SERVER_SELF_EXIT=True    PORT_2331_FREE
```

**⑥ 连续 3 次复位日志**（COM3 `115200/8N1/None/CRLF`，Latin1 逐字节读取；每次 `monitor reset` + `monitor go`；烧录本身后的首次捕获亦为同样内容）：

| 复位 | `LEN` | 首字节 | 结尾 2 字节 | 日志正文 |
| --- | --- | --- | --- | --- |
| #1 | 149 | `0xFC` | `0x0D 0x0A` | 3 次逐字一致 |
| #2 | 149 | `0xFC` | `0x0D 0x0A` | 同上 |
| #3 | 149 | `0xFC` | `0x0D 0x0A` | 同上 |

```text
[INF] STAGE3 FW=0.3.0 BUILD=Nov 12 2026 13:54:26 MCU=STM32F103ZET6 SYSCLK=72000000Hz UART=USART1/115200-8N1 BEEP=PB2(pin48)/active-high PWM_PATH=A\r\n
```

- `SYSCLK=72000000Hz` → **工作流附录 B #13 运行态验证闭环**（见 4.1 节）。
- **前导 `0xFC` 的判定**：3 次恒定、位于 `[INF]` 之前、仅 1 字节；按失败回退表逐项核对 —— HSE=8 MHz / PLL×9（运行态 SYSCLK 实测 72 MHz，波特率正确）、终端编码（ASCII/Latin1）、换行（CRLF）均无误；固件 `LOG_INFO` 格式串以 `[INF]` 开头，`debug_uart.c` 只做透传、无发送 `0xFC` 的代码；⑦ 断点实验中该字节甚至先于日志语句执行前就出现。故判定为**复位瞬间 RS232 线路的单个确定性毛刺**（MCU 复位期间 PA9 高阻，`U5 ST3232` 输入浮空→TX 电平跳变被 PC 侧 UART 采为 1 个杂散字节），**非固件输出、非波特率/编码问题、日志正文无乱码无截断**。已在 `docs/uart_wiring.md` 失败回退表登记，终端侧可忽略首字节或以 `[` 作为行起始判据。

**⑦ 断点单步**（GDB `-batch -x` 临时命令文件，在 `main.c:171` 设断点；未创建 `launch.json`）：

```text
Breakpoint 1 at 0x800051e: file .../Core/Src/main.c, line 171.
Breakpoint 1, main () at .../Core/Src/main.c:171
AT_BREAKPOINT_HIT=True     BEFORE_NEXT_LEN=1    BEFORE_NEXT_TEXT=<0xFC>   ← 无 [INF] 日志
AFTER_NEXT_SEEN=True       AFTER_NEXT_LEN=148   AFTER_NEXT_TEXT=[INF] STAGE3 ... PWM_PATH=A\r\n
SERVER_SELF_EXIT=True      PORT_2331_FREE
```

结论：断点停住时终端无日志，单步 `next` 之后才出现完整下一行 → **日志与执行顺序一致，可用于阶段 5 单步调试定位**。临时 GDB 命令文件、等待脚本与标记文件均位于 `$env:TEMP`，用完已删除。

---

## 7. 遗留项 L1–L9 状态更新（承接阶段 2 报告第 5 节）

| # | 遗留项 | 阶段 3 最终状态 | 处置归属 |
| --- | --- | --- | --- |
| L1 | halt `PC=0x1FFFF3B2` 提示 `JP8 BOOT0` 可能非 0 | ✅ **已闭环**：用户人工改 BOOT0 到 `2-3` 并重新上电，只读 halt 实测 `PC=0x08004FE6`（主 Flash），随后烧录成功并从 Flash 运行 | 已闭环 |
| L2 | 手册 `JP8/JP9` ↔ `BOOT0/BOOT1` 与锁定版相反 | 🟡 **差异仍存在但不阻塞**：BOOT0 已由实测闭环；按硬约束**未改写锁定版**、未据手册改跳线 | 人工目视 + 万用表记录差异（阶段 5 前） |
| L3 | `JP6` 当前为 JTAG 模式，与 SWD 目标冲突 | ⏸ 未闭环；本阶段未动跳线；但 `-if SWD -speed 4000` 已稳定完成连接、烧录、halt、断点单步，链路可用 | 人工断电切换；阶段 5 |
| L4 | J-Link V8 兼容/克隆类，FW 升级暂缓 | ✅ **保持「不执行」**；旧升级授权不沿用，未升级 FW、未改驱动 | 记录，不执行 |
| L5 | Debug 串口实际接线方式、`CON1` RS232 电平、`JP12` | 🟡 **基本闭环**：用户确认 `CON1` 接 RS232-USB 线且为 RS232 电平；实测 `COM3` FTDI `VID_0403/PID_6001` `Status OK`、3 次终端日志与断点单步均成功。`JP12` 现场目视仍未回传；另有复位前导字节 `0xFC` 已登记 | 人工目视 `JP12`（非阻塞） |
| L6 | 手册 CN1 针序与锁定版不同、板卡版本字符串未出现 | ⏸ 未闭环；不影响阶段 3（本阶段不使用 CN1） | 人工目视板卡丝印 |
| L7 | `JP5` / `J5` Vref / `Q1` / 蜂鸣器本体型号 / MCU 内核与容量 | ⏸ 未闭环；不影响阶段 3 | 人工复核；阶段 4/5 前 |
| L8 | `SystemCoreClock = 72 MHz` 运行态未读回 | ✅ **已闭环**：终端连续 3 次读到运行态 `SYSCLK=72000000Hz`，附录 B #13 达成（4.1 / 6.3 节） | 已闭环 |
| L9 | GDB Server 未打印 CPUID；阶段 5 需显式 `-device` + Flash 算法 + SVD | 🟡 **部分推进**：已用显式 `-device STM32F103ZE` 完成 `load` 烧录，J-Link 内置 Flash 算法实测生效（`Comparing/Erasing/Programming ... Done.`）；GDB Server 仍未打印 CPUID，SVD 外设视图仍未使用 | 阶段 5 |

---

## 8. 阶段 4 交接说明

**可直接 `@` 引用的输入**：`@docs/hardware_pins.md`（锁定版唯一硬件真相源）、`@docs/uart_wiring.md`、`@docs/stage3_report.md`、`@docs/clock_tree.md`、`@docs/init_selfcheck.md`、`@docs/stage2_build_report.md`、`@docs/stage2_hal_source.md`、`@docs/stage2_manual_notes.md`、`@.codebuddy/plans/阶段3-串口Debug输出_7c1e5a93.md`，以及 `Core/Inc/debug_log.h`、`Core/Src/debug_uart.c`、`Core/Src/main.c`、`CMakeLists.txt`。

**可直接复用的能力**：

- `printf` 已重定向到 `huart1`（USART1 115200 8N1），`setvbuf` 无缓冲保证日志与执行顺序一致（⑦ 已实测）；
- `LOG_ERROR/LOG_WARN/LOG_INFO/LOG_DEBUG` 四级宏与 `DEBUG_LOG_LEVEL` 编译期开关（Debug=4 / Release=2 / 可 `-D...=0` 全关）；
- `g_sysclk_hz/g_hclk_hz/g_pclk1_hz/g_pclk2_hz` 四个运行态频率快照（运行态已读到 72 MHz）、`g_tim2_update_count`（TIM2 10 Hz 基础时基，路径 A）；
- `build/Debug` 与 `build/Release` 两套可复现构建（CMake + Ninja + arm-none-eabi-gcc，Debug `-O0 -g3`）；
- 已验证可用的烧录 / 复位 / 断点单步链路：`-device STM32F103ZE -if SWD -speed 4000 -port 2331 -singlerun -nogui` + GDB `file/load/monitor reset/break/next`（阶段 4 若需再次烧录，仍按用户授权范围执行）。

**阶段 4 必须遵守的锁定事实与边界**：

- `{BUZZER_TYPE}` = 有源蜂鸣器、`{BUZZER_ACTIVE_LEVEL}` = 高电平有效、`{PWM_PATH}=A`（TIM 更新中断 + GPIO 软件 PWM，不改硬件）、`PB2`(pin48) 无 TIM 通道；按工作流 273 行，**有源蜂鸣器不得生成 PWM 变频改变音调的逻辑**；
- 沿用「零手写代码」，阶段 4 才实现鸣叫/节奏/占空比业务，且须非阻塞状态机；
- 继续禁止 ISR 内 `printf`、禁止高频刷屏（只在状态切换打印）、禁止 `HAL_Delay`；日志默认 DEBUG 等级 4，若单步调试困难可降到 2；
- 保持 PA13/PA14 调试功能，不做 SWJ remap / DBGMCU 关闭；Debug 构建固定 `-O0 -g3`；
- 新增外设时按 `@docs/stage2_hal_source.md` 第 4/6 节同步补官方 HAL `.c` 与 `stm32f1xx_hal_conf.h` 模块宏；
- 调试前端配置（`launch.json` / `.vscode/` / SVD 视图）属阶段 5，本阶段未创建；
- 当前板卡状态：`BOOT0=2-3`（User Flash），Flash 内为阶段 3 固件；`JP6` 仍为 JTAG 档但 SWD 实测可用；J-Link 为 V8 兼容/克隆类，不做 FW 升级。

---

## 9. 边界自检（如实记录做过与未做的事）

**已做（均在 B 授权范围内）**：

- 烧录 Flash 一次：GDB `load`，14,944 B，`Comparing/Erasing/Programming ... Done.`，未写 option bytes、未启用读保护；
- COM3 连续 3 次复位捕获启动日志，Latin1 逐字节核对；
- GDB 在 `main.c:171` 设断点并 `next` 单步，比对断点前后 COM3 内容；
- 每次 J-Link 会话 `SERVER_SELF_EXIT=True`、`PORT_2331_FREE`、无残留进程；临时 GDB 命令/标记脚本已删除。

**未做**：

- **AI 未改跳线**：`BOOT0` 跳线由用户人工改到 `2-3`，`JP5/JP6/JP9/JP12` 未动；未插拔 `J5`；
- 未改驱动、未终止任何用户进程、未升级 J-Link FW、未使用 J-Flash、未臆造 J-Link 参数；
- 未创建 `.vscode/` / `launch.json` / 任何调试前端配置（阶段 5）；
- 未实现蜂鸣器鸣叫/变频（阶段 4），未配置 PB2 为输出，未产生任何鸣叫；
- 未复用 PA13/PA14、未关闭 JTAG/SWD；Debug 构建 `-O0 -g3` 保留；
- 未修改工作流文档、锁定版硬件表、环境报告、`stage2_*` 文档、脚本、两个 PDF 与既有归档计划；`.codebuddy/` 未删除；
- **未虚报**：复位瞬间前导字节 `0xFC`、⑥ 的判定口径、`JP12`/`JP6` 等未闭环项均如实登记，不把硬件毛刺隐藏成「完全无杂散字节」，也不把 `JP6`/`JP12` 写成已确认。
