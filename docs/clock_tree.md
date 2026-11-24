# 阶段 2：时钟树配置与初始化顺序

> 依据：`@docs/hardware_pins.md`（锁定版，`{HSE_MHZ}=8`、`{SYSCLK_MHZ}=72`）、`@docs/stage2_hal_source.md`（官方 STM32CubeF1 V1.8.7）、官方 `stm32f1xx_hal_rcc.c` / `stm32f1xx_hal_rcc.h` / `stm32f103xe.h` 实测源码。
> 本文件对应工作流阶段 2 产出物第 1 项；不改写锁定版硬件映射表。

---

## 1. 时钟树（HSE → PLL → 72 MHz）

| 节点 | 配置 | 频率 | 依据 / 说明 |
| --- | --- | --- | --- |
| HSE | `Y2 8MHz`，`OSC_IN(23)/OSC_OUT(24)`，`C21/C22 20 pF` | 8 MHz | `@docs/hardware_pins.md` 42–43 行（P1 图纸事实） |
| PLL 输入预分频 | `RCC_HSE_PREDIV_DIV1`（`PLLXTPRE=/1`） | 8 MHz | `stm32f1xx_hal_rcc_ex.h` 449 行 `RCC_HSE_PREDIV_DIV1` |
| PLL 源 / 倍频 | `RCC_PLLSOURCE_HSE` + `RCC_PLL_MUL9`（×9） | 72 MHz | `stm32f1xx_hal_rcc.h` 94 行、`stm32f1xx_hal_rcc_ex.h` 547 行 |
| SYSCLK | `RCC_SYSCLKSOURCE_PLLCLK` | **72 MHz** | 目标值来自锁定版 74 行 |
| AHB / HCLK | `RCC_HCLK_DIV1` | 72 MHz | `RCC_ClkInitTypeDef.AHBCLKDivider` |
| APB1 / PCLK1 | `RCC_HCLK_DIV2` | 36 MHz | TIM2–TIM7、USART2/3、SPI2 等低速总线 |
| APB1 定时器时钟 | APB1 分频≠1 → 定时器时钟 ×2 | **72 MHz** | STM32F1 RCC 时钟树规则；`HAL_RCC_GetPCLK1Freq()` 返回 36 MHz，HAL 在计算 TIM 预分频时按 ×2 处理 |
| APB2 / PCLK2 | `RCC_HCLK_DIV1` | 72 MHz | USART1、GPIOA–G、AFIO、TIM1 所在总线 |
| APB2 定时器时钟 | APB2 分频=1 → 不倍频 | 72 MHz | TIM1（阶段 2 未启用） |
| FLASH | `FLASH_LATENCY_2`（2 wait states）+ 预取缓冲 `PREFETCH_ENABLE=1` | — | `stm32f1xx_hal_flash.h` 141 行；48 MHz < SYSCLK ≤ 72 MHz 时必需 |
| SysTick | `HAL_Init()` 以 `SystemCoreClock/1000` 配 1 ms；`HAL_RCC_ClockConfig()` 后按 72 MHz 重配 | 1 kHz | 官方 `stm32f1xx_hal_rcc.c` 941（更新 `SystemCoreClock`）、945（重新 `HAL_InitTick`）行实测 |

**USART1 波特率**：USART1 挂 APB2 = 72 MHz，`115200 8N1` → `USARTDIV = 72 000 000 / (16 × 115 200) = 39.0625`，`BRR = 0x271`（`mantissa=39`、`fraction=1`）。HAL 的 `HAL_UART_Init()` 依据 `HAL_RCC_GetPCLK2Freq()` 自动写入，本工程不手写 BRR。

**TIM2 更新中断（阶段 2 历史状态）**：TIM2 时钟 72 MHz → `PSC=7199`（/7200 → 10 kHz）→ `ARR=999`（/1000 → **10 Hz / 100 ms**）。阶段 2 回调只递增 `g_tim2_update_count`、**不翻转 PB2**。**阶段 4 已重新配置为 10 kHz 软件 PWM 波形发生器时基，推导见第 5 节。**

---

## 2. 初始化调用顺序

```text
Reset_Handler（官方 startup_stm32f103xe.s）
  → SystemInit()                 官方 CMSIS system_stm32f1xx.c，复位后 HSI 8 MHz
  → .data / .bss 初始化
  → __libc_init_array()
  → main()                      Core/Src/main.c（AI 生成）
      1. HAL_Init()              SysTick 1 ms 时基（此时 SystemCoreClock=HSI 8 MHz）
      2. SystemClock_Config()    HSE→PLL×9→72 MHz；HAL_RCC_ClockConfig 更新
                                 SystemCoreClock 并重配 SysTick（官方 hal_rcc.c 941/945）
      3. MX_GPIO_Init()          GPIOA/GPIOB/AFIO 时钟；PA9(AF_PP) / PA10(INPUT)；
                                 阶段 4 增补：PB2 推挽输出（复位 ODR2=0，上电即静音）
      4. MX_USART1_UART_Init()   115200 8N1；HAL_UART_MspInit 开 USART1 时钟
      5. MX_TIM2_Init()          TIM2 时基；HAL_TIM_Base_MspInit 开 TIM2 时钟 + NVIC
                                 （阶段 4 更新：PSC=71 / ARR=99 → 10 kHz，见第 5 节）
      6. HAL_TIM_Base_Start_IT() 启动更新中断（阶段 4 起 = 软件 PWM 波形发生器）
      7. 记录频率快照            g_sysclk_hz / g_hclk_hz / g_pclk1_hz / g_pclk2_hz
      8. 阶段 3 启动日志 + 阶段 4 Buzzer_Start()（装载段 1，随后追加一条 INFO）
      9. for(;;) Buzzer_Tick()   非阻塞 O(1) 序列推进器（阶段 2 为空转）
```

**SWD 硬约束**：`MX_GPIO_Init` 只用 `GPIO_PIN_9` / `GPIO_PIN_10` 的 pin mask 调用 `HAL_GPIO_Init(GPIOA, …)`，**PA13(SWDIO) / PA14(SWCLK) 保持复位默认**；工程内不存在 `__HAL_AFIO_REMAP_SWJ_*`、`__HAL_RCC_AFIO_CLK_DISABLE()` 或任何 DBGMCU 关闭调用；`AFIO->MAPR` 的 SWJ_CFG 位保持 000（JTAG+SWD 均可用）。

**蜂鸣器边界**：PB2 无 TIM 通道（锁定版 52 行）；阶段 2 不调用 `HAL_GPIO_WritePin`、不配置 PB2 为输出，避免与 `JP9 BOOT1` 网络冲突，也不提前实现阶段 4 业务。

---

## 3. 可观测性与调试前提

- `SystemCoreClock`（CMSIS 全局变量）与 `g_sysclk_hz` / `g_hclk_hz` / `g_pclk1_hz` / `g_pclk2_hz` 均可在 GDB / Cortex-Debug 变量窗口直接观察；Debug 构建 `-O0 -g3` 保留符号。
- `g_tim2_update_count` 用于确认 TIM2 更新中断向量与 ISR 链路可用。
- SVD 路径占位（阶段 5 才用）：`C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd`（`@docs/env_check.md` 8.3 实测）；本阶段不生成 `.vscode/` 或 `launch.json`。
- 串口日志属于阶段 3，本阶段不实现 `printf` / 发送逻辑；`CON1` 为 RS232 电平，接线方式仍按锁定版第 4 节 #7 待用户回传。

## 4. 来源与边界

| 文件 | 来源 |
| --- | --- |
| `Core/Src/system_stm32f1xx.c`、`startup_stm32f103xe.s`、`STM32F103ZETx_FLASH.ld` | ST 官方 `cmsis_device_f1`（原样复制，ld 仅改文件名），见 `@docs/stage2_hal_source.md` |
| `Core/Src/main.c`、`Core/Src/stm32f1xx_it.c`、`Core/Inc/*` | AI 生成；调用的 HAL API 与结构体字段均从官方 V1.8.7 头文件逐项核对 |
| `CMakeLists.txt`、`cmake/gcc-arm-none-eabi.cmake` | AI 生成；工具绝对路径取自 `@docs/env_check.md` 第 3 / 8.3 / 11 节实测输出 |

`{MCU_CORE}` / Flash / SRAM 容量在锁定版仍为「待人工确认」；链接脚本采用官方 `STM32F103xE` 模板的 512K FLASH / 64K RAM，属工程构建配置，不视为锁定结论的回填（手册补充证据见 `@docs/stage2_manual_notes.md` M1）。

---

## 5. 阶段 4 更新：TIM2 软件 PWM 时基（路径 A，2026-11-12）

TIM2 时钟仍为 **72 MHz**（APB1 分频≠1 → 定时器时钟 ×2，第 1 节规则不变）。为支撑蜂鸣器软件 PWM，阶段 4 重新配置 PSC/ARR：

| 项 | 阶段 2 历史 | 阶段 4 现值 | 说明 |
| --- | --- | --- | --- |
| PSC | 7199 | **71** | 72 MHz / (71+1) = **1 MHz** 计数频率 |
| ARR | 999 | **99** | 1 MHz / (99+1) = **10 kHz 更新中断**（恰为设计上限） |
| 用途 | 10 Hz 基础时基（只计数） | 软件 PWM 波形发生器 | 中断内只做「计数 + 比较 + PB2 置位/清零」 |

**推导与约束**（中断频率上限 10 kHz、占空比分辨率 ≥10 级、通断节拍 1–50 Hz 可配）：

1. `f_isr = 72 MHz / (PSC+1) / (ARR+1) = 72e6 / 72 / 100 = 10 kHz`，恰在 10 kHz 上限。
2. 软件 PWM 周期（ISR tick 数）= `10000 / 段频率`：段频率 1–50 Hz → 周期 **10000–200 ticks**；占空比每 1% 对应 **2–100 ticks** → 分辨率 **200–10000 级，≥10 级** 满足。
3. 中断负载：ISR 只有整数自增 + 两次比较 + 一次 `HAL_GPIO_WritePin`（单次 BSRR/BRR 写），约 100 cycle × 10 kHz ≈ **1.4% CPU**；**中断内无除法/浮点/printf**——除法与参数换算全部在主循环 `Buzzer_LoadSegment()` 完成，并用 PRIMASK 临界区写入共享参数。
4. `g_tim2_update_count++` 保留为 ISR 存活指示；PB2 波形由 `Buzzer_IsrUpdateOutput()` 产生（`Core/Src/buzzer.c`）。
5. 有效电平：高电平有效 → **静音 = PB2 常低**（`s_pwm_high_ticks=0`）；鸣叫段按占空比输出高/低，不反相。

**有源蜂鸣器语义**：段「目标频率」对有源蜂鸣器的实际听感是**断续节奏（响/停交替），不代表音调**；占空比决定每个节拍内高电平（响）时间占比，duty=100% 时为整段连续鸣叫（此时频率字段无可闻效果，接口保留）。默认序列、理论波形与实际听感见 `@docs/buzzer_sequence.md`。
