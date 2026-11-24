# 阶段 5：调试连接排查决策树与故障处置指南

> 项目：STM32F103ZET6 红牛开发板 ｜ 生成日期：2026-11-12 ｜ CodeBuddy IDE + PowerShell
> 依据：`STM32_Buzzer_PWM_Workflow.md` 569、623–634 行（失败回退表）、`docs/env_check.md` 第 5 节硬件禁忌与 10.1–10.4 排查史、`docs/hardware_pins.md`（锁定版）、阶段 2/3/4 报告遗留项。
> 硬约束：严禁 SWO/RTT 替代串口日志；严禁复用 PA13/PA14、严禁 SWJ remap / 关闭 DBGMCU；Debug 保持 `-O0 -g3`；J-Link FW 升级绝对禁止（V8 兼容/克隆类）；任何下载/烧录/GDB load 前需用户当轮显式授权。

---

## 1. 连接排查决策树（按顺序执行，勿跳步）

> 现象：J-Link 报 `Could not connect to target` / `Connecting to J-Link failed. Connected correctly?`

```text
T0 前置检查（软件侧，30 秒）
├─ 2331 端口是否被占用？（Get-NetTCPConnection -LocalPort 2331）
│    └─ 占用 → 关闭占用方（用户自行操作，不代杀进程）→ 重试
├─ 是否有 JFlash / 其他 JLink 进程占用调试器？（Get-Process JLink*,JFlash*）
│    └─ 有 → 用户自行关闭（阶段 0 教训：JFlash 连接目标时 GDB Server 争不到句柄）→ 重试
└─ 用已实证参数重试一次：-device STM32F103ZE -if SWD -speed 4000 -port 2331
     （出处：docs/env_check.md 354/372/418 行设置回显；禁止凭记忆改参数）

T1 查 JP6 JTAG_SEL 跳线（硬件侧第 1 项）
├─ 板卡断电 → 目视 JP6 是否为 SWD 可用模式（右侧引出 SWDIO/SWCLK）
├─ 记录：L3 遗留项曾为 JTAG 档，但阶段 3/4 实测 SWD 链路可用；
│   若连接失败且跳线在 JTAG 档 → 断电切 SWD → 重新上电 → 重试
└─ ⚠️ 严禁带电改跳线（硬件禁忌 #1）

T2 查 Vref（J5 pin1）
├─ GDB Server 日志读「Target voltage:」
│    ├─ 3.31 V → 正常（历史实测值）
│    ├─ 0 V / very low → 板卡未上电或 Vref 线断 → 查 JP4/J3 供电、J5 pin1 接线
│    └─ 数值异常波动 → 查 J5 pin1 接触
└─ Vref 无 3V3 时 J-Link 拒连或电平失配（硬件禁忌 #4）

T3 查共地与 J5 20pin
├─ J5 偶数脚（除 pin2=Vsupply）为 GND，必须与 J-Link 共地
├─ 20pin 排线方向、是否插到位（阶段 0 教训：排线未接回 = 目标侧失败，速率降速无效）
└─ 全部重新插拔前必须断电

T4 查 SWD 引脚是否被复用
├─ 代码层：Core/** 不得出现 PA13/PA14 GPIO 配置、__HAL_AFIO_REMAP_SWJ_*、DBGMCU 关闭
│   （阶段 2/3/4 静态自检均为 0 命中，回归检查即可）
├─ 运行层：若曾烧录过错误固件 → 用 J-Flash/GDB 全擦后重烧正确固件
└─ PA13(SWDIO)/PA14(SWCLK) 全程保持调试功能

T5 查 BOOT0=0（JP8）
├─ GDB halt 后读 PC：0x080xxxxx（主 Flash）= OK；
│   0x1FFFFxxx（系统存储器/bootloader）= BOOT0≠0
├─ 处置：断电 → 用户人工把 BOOT0 跳线改到 GND 档（2-3）→ 重新上电 → 重试
└─ 依据：阶段 3 实测 PC 由 0x1FFFF3B6 → 0x08004FE6 闭环；bootloader 区无法单步调试应用

T6 降速重试
├─ -speed 4000 → 1000 → 100 逐档重试（阶段 0 实测：速率交叉验证可排除速率因素）
└─ 若 -speed 100 成功 → 记录可用速率，保留低速配置；仍失败 → 回 T1 逐项复查
```

**每一步的记录要求**：记录 GDB Server 输出首行错误文本 + 当前跳线/接线状态，不盲目重试同一动作超过 2 次。

## 2. 下载/烧录失败

| 现象 | 处置 |
| --- | --- |
| `LoadFile` 报 Flash 编程错误 | 核对链接脚本 `STM32F103ZETx_FLASH.ld` 的 FLASH ORIGIN=0x08000000 / LENGTH=512K 与器件一致；确认 .hex 是最新构建产物 |
| `VerifyBin` 校验不一致 | 重试一次（擦写瞬时干扰）；仍失败 → 换 .hex 文件口径（`LoadFile .hex` 内嵌地址）单独烧录再校验；确认未启用读保护 |
| 疑似读保护 | 不写 option bytes、不解锁（有锁死风险）；报告用户人工评估 |
| BOOT0≠0 时烧录 | 烧录可能成功但复位后不运行应用 → 按 T5 处置 |
| J-Link 报固件升级弹窗 | **一律取消，严禁升级**（V8 兼容/克隆类，盲刷有变砖风险；阶段 0 遗留 L4） |

## 3. 断点不命中

按顺序核对：

1. Debug 构建为 `-O0 -g3`：`build/Debug/build.ninja` 的 `FLAGS` 实测核对（阶段 4 已实测含 `-O0 -g3`）。
2. `.elf` 与源码一致：重新 `cmake --build build/Debug` 后确认 `.elf` 时间戳晚于最后一次源码改动；烧录的固件与加载符号的 `.elf` 必须是同一份产物。
3. 符号名正确：`arm-none-eabi-nm build/Debug/RedBull_Buzzer_PWM.elf | Select-String Buzzer_Tick` 应有 `T` 类型条目。
4. 断点落在有效代码行：优先 `Buzzer_Tick()` 函数入口或其段切换分支（`buzzer.c` `Buzzer_LoadSegment(next, now)` 调用行，阶段 4 实测可稳定命中）；`main()` 中 `runToEntryPoint` 未跳过目标函数。
5. Flash 与 RAM 断点：`Buzzer_Tick()` 在 Flash（0x08000xxx），用 FlashBP 特性（本调试器 Feature(s) 实测含 FlashBP）。

## 4. 变量显示 optimized out

1. 确认 Debug 构建 `-O0 -g3`（见 3.1）。
2. 确认变量为 `volatile`：`g_buzzer_seg_index/freq_hz/duty_pct/remain_ms` 在 `Core/Inc/buzzer.h` 39–42 行均为 `volatile uint32_t`（阶段 4 已核对，勿改动）。
3. 重新构建并重烧后观察。
4. 兜底：直接用地址 print（阶段 4 nm 实测地址口径，地址随构建可能微移，先重新 nm）：
   ```gdb
   p/x &g_buzzer_seg_index        # 取地址
   x/4xw &g_buzzer_seg_index      # 连续读四变量
   ```

## 5. SVD 外设视图打不开

1. `svdFile` 回填实测绝对路径（勿按占位符拼路径，SVD 实际文件名是 `STM32F103xx.svd`）：
   `C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd`（867,312 字节，`docs/env_check.md` 8.3）。
2. 确认文件仍存在：`Get-Item` 校验。
3. 仍打不开 → 用 GDB 内存读寄存器替代（地址已逐项对 SVD 实测）：
   | 寄存器 | 地址 | 期望值 |
   | --- | --- | --- |
   | `TIM2->PSC` | `0x40000028` | 71（0x47） |
   | `TIM2->ARR` | `0x4000002C` | 99（0x63） |
   | `RCC->CFGR` | `0x40021004` | 0x0000040A（SW=PLL、SWS=PLL、PPRE1=/2） |
   | `GPIOB->ODR` | `0x40010C0C` | bit2 = 蜂鸣器当前电平 |
   | `USART1->BRR` | `0x40013808` | 0x271（115200 @72 MHz） |
   | `x/1xw 0x00000004` | 复位向量 | 0x0800xxxx（指向 Flash） |

## 6. 内置终端 GDB 命令报错

1. 逐条重跑并回传完整错误文本（不跳过报错行）。
2. 确认 GDB Server 端口与 GDB `target remote` 端口一致（均 2331）。
3. 确认 `arm-none-eabi-gdb` 用实测绝对路径：`C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-gdb.exe`。
4. `-singlerun` 模式下 GDB 断开后 Server 自动退出，属预期（每次调试需重启 Server，脚本已含此逻辑）。

## 7. 单步调试操作清单（主选：CodeBuddy 图形化，配置见 `.vscode/launch.json`）

> 主选可用性探测结论见 `docs/stage5_report.md` 第 2 节；以下任一步失败按第 1–6 节处置，两次失败切兜底（`scripts/debug_gdb_cmds.txt`）。

1. **先开串口终端**：COM3、115200 8N1、无流控、CRLF 显示（配置清单见 `docs/uart_wiring.md` 第 1 节）。调试全程保持打开——单步暂停时串口停止输出新日志是共用硬验收项。
2. **F5 启动**：`preLaunchTask` 先执行 CMake Debug 构建 → Cortex-Debug 启动 `JLinkGDBServerCL` → 连接 SWD → 下载 `.elf` → `runToEntryPoint: "main"` 停在 `main()`。调试控制台（DEBUG CONSOLE）应无 error。
3. **设断点**：在 `Core/Src/buzzer.c` 的 `Buzzer_Tick()` 内（如 `Buzzer_LoadSegment(next, now);` 行）单击行号左侧；F5 继续运行后**断点稳定命中**。
4. **F10 单步**：逐句推进；每次段到期单步过 `Buzzer_LoadSegment()` 后观察变量变化。
5. **变量视图**（VARIABLES 面板）观察四个状态变量：`g_buzzer_seg_index`（0→1→…→7→0 循环）、`g_buzzer_freq_hz`/`g_buzzer_duty_pct`（响段 2/100 ↔ 停段 0/0）、`g_buzzer_remain_ms`（随 tick 递减）。
6. **SVD 外设视图**（XPERIPHERALS，来自 `mcu-debug.peripheral-viewer`）：打开 `svdFile` 指向的 STM32F103xx.svd，查看 TIM2（PSC=71/ARR=99）、RCC（CFGR SWS=PLL）、GPIOB（ODR bit2）、USART1（BRR=0x271）实时值。
7. **串口同步观察**：单步暂停时 COM3 停止输出新日志；continue/恢复后下一条段切换 INFO 出现——证明调试与日志同步（共用硬验收项 ④）。
8. 结束：Shift+F5 停止调试；Server 随会话关闭。

## 8. 兜底操作清单（命令行 GDB）

见 `scripts/debug_gdb_cmds.txt`（每条命令附预期输出）。局限性：无图形化变量视图/SVD 树，但**断点命中、变量值、寄存器值均可从终端输出观测**；单步用 `next`（F10 等价）；串口观察仍用 COM3 终端（不因命令行调试而改变）。

## 9. 操作禁忌（每次调试会话都适用）

- 严禁带电插拔 J5 20pin / 改跳线；改 JP6/JP8 前必须断电。
- 严禁 SWO/RTT 替代串口日志（本项目 Debug 输出唯一通道 = USART1 COM3）。
- 严禁复用 PA13/PA14、严禁 `__HAL_AFIO_REMAP_SWJ_*` 或关闭 DBGMCU。
- 严禁 J-Link FW 升级；严禁写 option bytes / 启用读保护。
- 严禁终止用户已开启的进程（J-Link 占用冲突时由用户自行关闭）。
- 烧录/下载/GDB load 仅在用户当轮显式授权后执行。
