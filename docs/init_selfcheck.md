# 阶段 2：初始化自检清单（上电后可观测的 5 个事实）

> 对应工作流阶段 2 提示词产出物第 6 项与验收门第 359 行「自检清单 5 项均写出可观测方式」。
> 真相源：`@docs/hardware_pins.md`（锁定版）、`@docs/clock_tree.md`、`@docs/env_check.md`（J-Link / SVD 实测）。
> **阶段 2 不烧录、不改跳线**；以下事实在后续烧录后 / 调试态验证，本阶段只完成构建与 J-Link 识别 + halt。

---

## 自检表

| # | 应能观测到的事实 | 期望值 | 可观测方式（GDB / Cortex-Debug） | 若不符时回退 |
| --- | --- | --- | --- | --- |
| 1 | `SystemCoreClock` 已被 HAL 更新为 72 MHz | `72000000` | `p SystemCoreClock`；对照 `p g_sysclk_hz`（`HAL_RCC_GetSysClockFreq()`）与 `p g_hclk_hz` 均为 72000000；SVD 视图读 `RCC->CFGR.SWS = PLL` | 按工作流阶段 2 失败回退：用 SVD/GDB 读 `RCC` 寄存器核对 `PLLSRC/PLLMUL/SW`；不臆断时钟 |
| 2 | 总线分频正确 | `g_hclk_hz=72000000`、`g_pclk1_hz=36000000`、`g_pclk2_hz=72000000` | `p g_hclk_hz`、`p g_pclk1_hz`、`p g_pclk2_hz`；SVD 读 `RCC->CFGR` 的 `HPRE=0`、`PPRE1=/2`、`PPRE2=/1` | 检查 `SystemClock_Config()` 的 `RCC_ClkInitTypeDef`；`FLASH->ACR` 应为 `LATENCY_2` |
| 3 | `USART1` 已按 115200 8N1 初始化，且 TX/RX 引脚配置正确 | `huart1.Instance=USART1`、状态 READY、`USART1->BRR = 0x271` | `p huart1`；`x/1xw &USART1->BRR`（或 SVD 外设视图）；读 `GPIOA->CRH` 确认 PA9=AF_PP、PA10=INPUT；`p/x huart1.ErrorCode` 应为 0 | 电平/接线陷阱：板上 `CON1` 经 `U5 ST3232` 为 **RS232 电平**，配 RS232-USB 线；USB-TTL 须从扩展排针取 PA9/PA10 TTL 并共地（锁定版 59、146 行） |
| 4 | SysTick 1 ms 时基可用 | `SysTick->LOAD = 71999`，`HAL_GetTick()` 递增 | 两次 `p HAL_GetTick()`（间隔约 1 s 应增加约 1000）；`x/1xw 0xE000E014` 读 `SYST_RVR`；`p uwTickFreq` | 检查 `HAL_InitTick()` 是否在 `HAL_RCC_ClockConfig()` 后按 72 MHz 重配（官方 `hal_rcc.c` 945 行） |
| 5 | 路径 A TIM2 更新中断链路可用，且蜂鸣器 / SWD 未被误动 | `g_tim2_update_count` 以约 10 Hz 递增；PB2 保持未被驱动；J-Link 仍可 halt | `p g_tim2_update_count`（两次读取间隔 1 s 约 +10）；SVD 读 `TIM2->SR.UIF` 与 NVIC `ISER` 中 TIM2_IRQn；`x/1xw 0x40010C00`（`GPIOB->CRL`）确认 `MODE2=00`、`x/1xw 0x40010C0C`（`GPIOB->ODR`）bit2=0；`AFIO->MAPR` 的 SWJ_CFG 保持 000 | 计数不增 → 查 `HAL_TIM_Base_Start_IT()` / `HAL_TIM_Base_MspInit()` / `TIM2_IRQHandler`；PB2 被驱动或 SWJ_CFG 变化 → 立即停止，检查是否误加 GPIO/remap 配置 |

---

## 观测前提与边界

- Debug 构建为 `-O0 -g3`，保留符号；`SystemCoreClock` 与 `g_*` 变量均为 `volatile`/全局变量，可在调试窗口直接观察。
- 串口日志属于**阶段 3**，本阶段以上事实通过调试器变量与 SVD 外设视图验证，不实现 `printf`、不启用 SWO。
- SVD 路径占位（阶段 5 图形化视图使用）：`C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd`（`@docs/env_check.md` 8.3 实测，867312 字节）；阶段 5 仍需显式 `-device STM32F103ZE`。
- J-Link 只做识别与 halt；`pc = 0x1FFFF3B6` 提示 `JP8 BOOT0` 可能非 0，`JP6` 当前为 JTAG 模式，均属人工复核项（见 `@docs/stage2_manual_notes.md` R1/R2/R3），本阶段不改跳线、不烧录。
- 蜂鸣器为**有源、高电平有效**（`@docs/hardware_pins.md` 50–51 行，11-12 GDB 实测闭环）；阶段 2 不产生任何鸣叫，自检项 5 的「PB2 未被驱动」正是防止提前进入阶段 4 的守门条件。
