# 阶段 0：资料与环境准备 —— 环境就绪报告

> 项目：STM32F103ZET6 红牛开发板，蜂鸣器 PWM 鸣叫，`USART1` 作为 Debug 输出，J-Link SWD 单步调试。
> 生成日期：2026-11-11 ｜ 平台：Windows + PowerShell ｜ 开发前端：CodeBuddy IDE
> 依据：`STM32_Buzzer_PWM_Workflow.md`（CodeBuddy IDE 版）第 84–171 行「阶段 0」六要素与附录 B/F。

---

## 0. 交付边界与真相源

**本次只完成阶段 0**，产出：

```
docs/env_check.md          # 本文件：环境校验命令清单、对照表、判定规则、禁忌、验收门、失败回退、实测记录
scripts/env_check.ps1      # 一键只读校验脚本
```

**没有生成、也不会在阶段 0 生成**：任何 `.c/.h/.s/.ld`、`CMakeLists.txt`、`.ioc`、`launch.json`/调试配置、固件代码；未修改 `STM32_Buzzer_PWM_Workflow.md` 与原理图 PDF；**未解析原理图**（文档第 97、107 行明确本阶段仅确认可读）；未进入阶段 1。

**硬件真相源**（文档阶段 1 引脚映射表，本阶段仅用于硬件禁忌与接线预检，**不做任何引脚配置、不生成代码**）：

| 字段 | 取值 |
| --- | --- |
| 蜂鸣器网络 | `BEEP/BOOT1` = PB2 / LQFP144 pin 48，`JP5` 使能，5V 供电 |
| Debug 串口 | `USART1`，PA9(TX,101) / PA10(RX,102)，115200 8N1 |
| 串口电平 | 板上 `CON1`/`CON2` 经 `U5 ST3232` 为 **RS232 电平，不是 TTL** |
| SWD | PA13(SWDIO,105) / PA14(SWCLK,109)，`J5` 20pin ARM JTAG，`JP6 JTAG_SEL` |
| 跳线 | `JP5` BEEP、`JP6` JTAG_SEL、`JP8` BOOT0、`JP9` BOOT1（注意与 PB2 冲突）、`JP4` Power |

---

## 1. 一键校验命令（验收入口）

在 CodeBuddy 内置终端、工作区根目录执行（AI 生成，用户只需复制运行）：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\scripts\env_check.ps1"
```

脚本特性：**只读、幂等、无网络、不写系统状态**。它只在自己所在的子进程内把 4 个实测 `bin` 目录追加到 `$env:PATH`（关闭即失效，不改用户级/机器级环境变量），输出每行带 `MUST/OPTIONAL/INFO` + `PASS/FAIL` + 版本号或绝对路径，可整段复制回传给 AI。

**本次实测输出摘要**：

```
MUST_PASS=9 MUST_FAIL=0
MISSING_ITEMS=none
VERDICT_HINT=ALL_MUST_PASS
```

完整输出见第 11 节。按文档第 159 行，用户把输出回传给 AI，由 AI 给出「必须项全部就绪」或具体缺项 —— 本次判定见第 10 节。

---

## 2. 环境校验命令清单（PowerShell 片段）

### 2.1 构建工具与交叉编译器（复用文档第 136–144 行「验证点」，本机适配）

```powershell
Get-Command cmake, ninja, arm-none-eabi-gcc, arm-none-eabi-gdb, JLinkGDBServerCL -ErrorAction SilentlyContinue |
    Select-Object Name, Source
cmake --version
ninja --version
arm-none-eabi-gcc --version
arm-none-eabi-gdb --version
arm-none-eabi-size --version
```

> 注意：`cmake` 与 `ninja` 是阶段 0 解压到用户目录的可移植包，**不在持久 PATH 中**。裸跑上面四条 `--version` 前，需先执行第 9 节的会话级 PATH 补齐命令，或直接运行第 1 节的一键脚本（脚本自动补齐）。

### 2.2 J-Link、SVD、CubeMX、资料

```powershell
Get-ChildItem "C:\Program Files\SEGGER" -ErrorAction SilentlyContinue | Select-Object Name
Get-ChildItem "C:\Program Files\SEGGER\JLink_V818" -Filter "JLinkGDBServerCL.exe" -ErrorAction SilentlyContinue |
    Select-Object FullName, Length
Get-ChildItem "C:\Program Files\STMicroelectronics" -ErrorAction SilentlyContinue | Select-Object Name
Get-ChildItem "C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD" `
    -Filter "STM32F103*.svd" -ErrorAction SilentlyContinue | Select-Object FullName, Length
Get-Item "红牛开发板电路图(RedBull Schematic).pdf" | Select-Object Name, Length
```

> `JLinkGDBServerCL.exe` **只探测文件、不执行**：J-Link 命令行参数须在阶段 5 先跑 `--help` 探测再确定（文档第 559 行硬约束），阶段 0 不臆造参数。
> SVD 的实际文件名是 `STM32F103xx.svd`，故用通配 `STM32F103*.svd`。

### 2.3 CodeBuddy 侧探测（文档第 146–151 行）

按文档要求「不臆造命令，由 AI 按本机实际情况生成探测步骤、在内置终端执行并回传结果」，本阶段实测如下：

| 探测项 | 探测方式 | 本次实测结论 |
| --- | --- | --- |
| 侧栏 `Plan Mode` 可进入且能「新建计划」 | 本次会话由 `Plan` 模式新建「阶段 0」计划 → READY 审阅 → 「开始执行」 | **已实证可用**，且计划已落到 `.codebuddy/plans/` |
| 输入框左下 `Ask ▾` 可切 `Plan`/`Craft`/`Ask` | 本次会话已发生 `Plan → Craft` 切换并进入执行 | **已实证可用** |
| 内置终端可执行 PowerShell 且输出可回传 | 本阶段全部探测/安装/复验命令均由 AI 在内置终端执行，stdout 逐字回传 | **已实证可用** |
| `@` 可引用本文档与原理图 PDF | 用户以 `@STM32_Buzzer_PWM_Workflow.md` 引入，AI 成功读取 904 行全文 | **已实证可用** |
| 插件市场可搜索 `Cortex-Debug` | 界面项，脚本无法替代；改为实测扩展宿主目录 | **扩展已安装**：`C:\Users\HenrryBian\.codebuddycn\extensions\marus25.cortex-debug-1.13.0-pre6-universal`，`package.json` 显示 `version=1.13.0-pre6`，4 个 `mcu-debug.*` 依赖全部存在 |

**界面复核步骤（建议用户做，不阻塞阶段 0）**：扩展面板搜索 `Cortex-Debug`，确认显示为「已安装/已启用」并截图回传。
**若搜不到**：按官方 FAQ 在设置中配置 `marketplace.visualstudio.com` 后重启（本次实测 `C:\Users\HenrryBian\AppData\Roaming\CodeBuddy CN\User\settings.json` 中尚无该配置，但扩展已安装，故不影响主选结论）；仍不可用则记录「主选不可用」，阶段 5 直接走命令行 GDB 兜底 —— 按文档第 157 行，两种结论都不阻塞阶段 0。

**其他 CodeBuddy 事实**：IDE 进程实测路径 `D:\Program Files\CodeBuddy CN\CodeBuddy CN.exe`；扩展宿主 `C:\Users\HenrryBian\.codebuddycn\extensions`（已装 `tomoki1207.pdf-1.2.2-universal`，阶段 1 的 `pdf` 能力可用）；工作区 `.codebuddy/plans/` 已存在并有归档计划文件。

---

## 3. 工具对照表（工具 / 用途 / 预期版本输出 / 缺失时安装方式）

| 工具 | 用途 | 本机实测版本与绝对路径 | 缺失时安装方式 |
| --- | --- | --- | --- |
| `cmake` | 构建系统生成 | `cmake version 4.5.0-rc3`；`...\EmbeddedTools\cmake_x64\cmake-4.5.0-rc3-windows-x86_64\bin\cmake.exe` | `winget install --id Kitware.CMake --exact --source winget --silent --accept-package-agreements --accept-source-agreements --disable-interactivity`（下载源 `github.com`，本次超时）；替代见 7.2，从 Chocolatey `cmake.portable` 取可移植包，或 `cmake.org` 官方 zip 断点续传 |
| `ninja` | 构建后端 | `1.13.2`；`...\EmbeddedTools\ninja\bin\ninja.exe` | `winget install --id Ninja-build.Ninja ...`（下载源 `github.com`，本次超时）；替代见 7.2，Chocolatey `ninja` 包内嵌 `tools\ninja-win_x32.zip` |
| `arm-none-eabi-gcc` | 交叉编译 | `14.2.1 20241119`（Arm GNU Toolchain 14.2.Rel1）；`C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-gcc.exe` | `winget install --id Arm.GnuArmEmbeddedToolchain --exact --source winget --silent --accept-package-agreements --accept-source-agreements --disable-interactivity`（**不要加 `--scope user`**，NSIS 安装器不支持）；或 Arm 官方 GNU Arm Embedded Toolchain 安装包 |
| `arm-none-eabi-gdb` | 命令行单步（兜底路径） | `15.2.90.20241130-git`；同上 `bin` 目录 | 与 gcc 同包附带，无需单独安装 |
| `arm-none-eabi-size` | Flash/RAM 占用报告（阶段 2） | `2.43.1.20241119`；同上 `bin` 目录 | 与 gcc 同包附带 |
| `JLinkGDBServerCL` | J-Link GDB Server（阶段 5） | 文件 590288 字节；`C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe` | `winget install --id NordicSemiconductor.JLink ...`（Nordic 分发的 SEGGER J-Link Software and Documentation Pack 8.18.0，本次安装器返回 exit 2 但目标文件已完整就位）；或 SEGGER 官方 J-Link 软件包。**记录绝对路径，不依赖 PATH** |
| `STM32F103xx.svd` | Cortex-Debug 外设寄存器视图 | 867312 字节；`...\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd` | 从 `https://www.keil.com/pack/index.idx` 查版本 → 下载 `Keil.STM32F1xx_DFP.2.4.1.pack` → 解压 `SVD/` 目录；或从 STM32CubeMX / Keil Pack 安装目录搜索 `STM32F103*.svd`，把实际路径回填 |
| `Cortex-Debug` 扩展 | 阶段 5 图形化单步**主选** | `marus25.cortex-debug 1.13.0-pre6` + 4/4 依赖；`C:\Users\HenrryBian\.codebuddycn\extensions\` | CodeBuddy 扩展面板搜索安装；搜不到按官方 FAQ 配置 `marketplace.visualstudio.com`；仍不可用则阶段 5 走命令行 GDB 兜底 |
| CodeBuddy IDE | 开发前端 | `D:\Program Files\CodeBuddy CN\CodeBuddy CN.exe` | 官方安装包；确认已登录、`Plan Mode` 可用 |
| `STM32CubeMX` | 图形化改配置 / 生成 HAL（**软件本身可选**） | 未安装，winget 源无匹配包 | ST 官网官方安装包人工安装。`.ioc` 在阶段 2 为可选，但 **STM32F1 HAL 驱动源是阶段 2 的必须输入**（文档第 291 行）：装 CubeMX 是取得完整驱动集最省事的途径；另一途径是由 AI 从 ST 官方 STM32CubeF1 固件包 / CMSIS 渠道补齐 `stm32f1xx_hal_*.c`，具体来源与路径须实测，见第 10 节 |
| `winget` | 包管理 | `v1.29.380`；`C:\Users\HenrryBian\AppData\Local\Microsoft\WindowsApps\winget.exe` | 随 Windows App Installer 提供 |
| 原理图 PDF | 阶段 1 输入资料 | 114821 字节，`OpenRead` 成功 | 核对文件名 `红牛开发板电路图(RedBull Schematic).pdf` 且可读 |
| 示波器 / 逻辑分析仪 | 波形频率与占空比核验 | 未探测（**可选项**） | 可用 J-Link + 变量观察 + 日志替代（文档第 89、533 行） |

---

## 4. 依赖就绪判定规则

### 4.1 必须项（对应一键脚本的 9 条 `MUST`）

| # | 必须项 | 可观测通过条件 | 对应验收标准 |
| --- | --- | --- | --- |
| 1 | `cmake` | `--version` 返回有效版本号 | 第 155 行 |
| 2 | `ninja` | `--version` 返回有效版本号 | 第 155 行 |
| 3 | `arm-none-eabi-gcc` | `--version` 返回有效版本号 | 第 155 行 |
| 4 | `arm-none-eabi-gdb` | `--version` 返回有效版本号 | 第 155 行 |
| 5 | `JLinkGDBServerCL.exe` | 能定位绝对路径且文件存在（不执行） | 第 158、167 行 |
| 6 | STM32F1 `.svd` | 能定位绝对路径且文件存在 | 第 158、166 行 |
| 7 | CodeBuddy IDE | 安装路径/进程可定位 | 第 156 行 |
| 8 | `Cortex-Debug` 扩展结论 | 「已安装/可安装 → 阶段 5 主选」或「不可用 → 命令行 GDB 兜底」，二者皆算通过 | 第 157 行 |
| 9 | 原理图 PDF | 存在、可读、字节数 > 0 | 第 171 行 |

**补充必须项（界面实证，脚本不覆盖）**：能新建 `Plan`、能切 `Craft`、内置终端能执行 PowerShell 且输出可回传、`@` 能引用本文档 —— 本次会话全部实证（见 2.3）。

### 4.2 可选项（缺失不阻塞阶段 0）

- `STM32CubeMX`（**软件本身**）：文档第 327、339 行把 `.ioc` 标为「可选」；阶段 0 验收标准（153–159 行）与附录 B #1/#2/#3 均不含它，第 120 行只要求探测并输出版本号。
- **注意**：可选的只是 CubeMX 这个软件，**不是 HAL 驱动源**。文档第 291 行把「STM32F1 HAL 与 CMSIS Pack」列为阶段 2 的必须输入；本次实测 `Keil.STM32F1xx_DFP.2.4.1` 只提供 `STM32F103xx.svd` 与 StdPeriph 库（`stm32f10x_*.h/.c`），`stm32f1xx_hal_*.c` 计数为 **0**，HAL 源尚未就绪，阶段 2 准入前必须补齐，详见第 10 节。
- 示波器 / 逻辑分析仪：用 J-Link + 变量观察 + 日志替代。
- VS Code CLI（`code`）：仅作对照，开发前端是 CodeBuddy IDE。
- `arm-none-eabi-size`：阶段 2 才用，本次已随工具链附带。

### 4.3 判定规则

1. 运行第 1 节一键命令，回传完整输出。
2. 9 条 `MUST` 全为 `PASS` → AI 回复「**必须项全部就绪**」；任一 `FAIL` → AI 列出具体缺项并指向第 7 节对应回退动作，回退后重跑一键命令。
3. `OPTIONAL MISSING` 只记录，不改判定。
4. 「保持 SWD 可调试」属于硬件与配置的持续约束：阶段 0 只装软件、不接板、不上电、不烧录、不改跳线、不写 option bytes、不启用读保护；任何后续阶段都不得复用 PA13/PA14 或关闭调试口。

---

## 5. 硬件操作禁忌清单（绝对不能做）

| # | 禁忌 | 原因 / 后果 | 正确做法 | 真相源字段 |
| --- | --- | --- | --- | --- |
| 1 | **带电插拔 JTAG/SWD 连接器** | 可能损坏调试器与 MCU 引脚 | 断电后再插拔 `J5` 20pin 与 J-Link 转接 | `J5` 20pin，`JP6 JTAG_SEL` |
| 2 | **把 5V 接到 3.3V 引脚** | 烧毁 MCU；板上 5V 仅供蜂鸣器（`DZ1`） | 5V 只接蜂鸣器支路；MCU 侧一律 3V3 | 蜂鸣器 5V 供电、3V3 数字/模拟分区 |
| 3 | **把 RS232 电平的 `CON1` 直连 USB-TTL** | `U5 ST3232` 输出 ±3~15V，会损坏 TTL 串口与 MCU | 用 `CON1` 必须配 **RS232-USB** 线；用 USB-TTL 则从扩展排针取 `PA9/PA10` TTL 并**共地** | `CON1`/`CON2` 为 RS232 电平；USART1=PA9/PA10 |
| 4 | **不共地 / `Vref` 未接 3V3** | J-Link 无法识别目标或电平失配 | `J5` pin1 `Vref` 接 3V3，偶数脚 GND 全部共地 | `J5` pin1 Vref=3V3、偶数脚 GND |
| 5 | **`JP6 JTAG_SEL` 模式与 J-Link 不匹配** | SWDIO/SWCLK 无效，J-Link 报连不上 | 对照板上丝印与原理图 P1 右下，设为 SWD 可用模式 | `JP6 JTAG_SEL` |
| 6 | **`JP8 BOOT0` 不为 0** | 不从主 Flash 启动，单步调试与烧录后运行异常 | `BOOT0=0`（从主 Flash 启动），单步必需 | `JP8 BOOT0` |
| 7 | **`JP9 BOOT1` 把 PB2 强拉到固定电平** | 蜂鸣器网络 `BEEP/BOOT1` 与 PB2 共用，会导致 PWM/鸣叫失效 | 核对跳线，确认 PB2 仍可由 GPIO 驱动 | `JP9 BOOT1`、`BEEP/BOOT1`=PB2 |
| 8 | **复用或关闭 SWD 引脚** | PA13/PA14 被改成 GPIO 或调试口被 remap/DBGMCU 关闭后，J-Link 全程失联 | 保持 PA13(SWDIO)/PA14(SWCLK) 调试功能，禁止 remap/关闭配置 | SWD=PA13/PA14 |
| 9 | **写 option bytes / 启用读保护** | 可能锁死芯片，破坏可调试性 | 阶段 0 及后续均不执行任何 option bytes 改写、不启用读保护 | — |
| 10 | **飞线前不断电** | 短路/带电焊接损坏板卡 | 路径 B/C 飞线到 `CN1` 的 `PWM0`(PB0/TIM3_CH3) 前**必须断电**，并断开原 PB2 侧 | `CN1` 扩展口 |
| 11 | **`JP5` 未短接却判定蜂鸣器故障** | 蜂鸣器永远不响，会误判成代码 bug | 断电目视确认 `JP5 BEEP` 短接 | `JP5` 使能 |

**阶段 0 安全边界**：本次仅安装/下载软件与资料文件，未连接开发板、未上电、未烧录、未改动任何跳线，也未改动 option bytes 或读保护位 —— 满足「保持 SWD 可调试」硬约束。

---

## 6. 阶段 0 验收门（可观测通过条件）

| # | 验收项 | 可观测证据 | 本次结果 |
| --- | --- | --- | --- |
| 1 | `cmake`/`ninja`/`arm-none-eabi-gcc`/`arm-none-eabi-gdb` 四条 `--version` 均返回有效版本号，无「不是内部或外部命令」 | 一键脚本 `MUST PASS` 四行 + 版本号 | ✅ |
| 2 | 能新建 `Plan`、切 `Craft`、内置终端能执行 PowerShell 且输出可回传、`@` 能引用本文档 | 本次会话 Plan→READY→Craft、探测命令 stdout、904 行文档读取 | ✅ |
| 3 | `Cortex-Debug` 扩展可用性有明确结论，且两种结论都不阻塞阶段 0 | 扩展目录 + `package.json` + 4/4 依赖 | ✅ 主选可用 |
| 4 | 能定位 J-Link 安装目录（存在 `JLinkGDBServerCL.exe`）与 STM32F1 `.svd` 绝对路径 | 两条 `INFO *_ABS=` 行 | ✅ |
| 5 | 用户把校验输出回传，AI 明确回复「必须项全部就绪」或列出具体缺项 | 第 10 节判定 | ✅ |
| 6 | 附录 B 阶段 0 勾选项 #1/#2/#3 | 本报告 + 脚本输出 | ✅ |
| 7 | 附录 B #33「每阶段一个独立 Plan，READY 审阅后才开始执行，完成后归档」 | 本阶段独立 Plan 已 READY 并开始执行；`.codebuddy/plans/` 机制可用；归档待阶段 0 收尾 | 进行中 |

**附录 B 阶段 0 勾选项**：☑ #1 `cmake`/`ninja`/`arm-none-eabi-gcc` 有版本号（本次另有 `arm-none-eabi-gdb`，共四条）；☑ #2 CodeBuddy 环境可用；☑ #3 J-Link 与 SVD 路径已定位。
**#33**：本阶段已用独立 Plan 且经 READY 审阅后开始执行；「完成后归档」在阶段 0 收尾时由 IDE 完成，故暂标记「进行中」，不作为本阶段未通过项。

**未通过则不得进入阶段 1**；按第 7 节回退后重跑第 1 节一键命令。

---

## 7. 失败回退表

### 7.1 文档原表（第 161–171 行）+ 本机补充

| 现象 | 回退动作 |
| --- | --- |
| `arm-none-eabi-gcc` 找不到 | 执行 3 节表格中的 winget 命令安装 `Arm.GnuArmEmbeddedToolchain`（**不加 `--scope user`**），并把 `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin` 追加到当前会话 `$env:PATH` 后重验 |
| 找不到 `.svd` | 从 STM32CubeMX / Keil Pack 安装目录搜索 `STM32F103*.svd`；本机已实测的补齐命令见 7.3；把实际路径回填到本报告与阶段 5 `svdFile` |
| `JLinkGDBServerCL` 不在 PATH | **记录绝对路径**（本机 `C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe`），后续所有调试配置使用绝对路径，不依赖 PATH |
| 侧栏找不到 `Plan Mode` / 无法新建计划 | 确认使用 CodeBuddy IDE 版并已登录；点输入框左下 `Ask ▾` 查看模式下拉；开 `New Chat` 新会话；把界面截图回传 AI 定位 |
| 插件市场搜不到 `Cortex-Debug` | 按官方 FAQ 在设置中配置 `marketplace.visualstudio.com` 后重启；仍不可用则记录「主选不可用」，阶段 5 直接采用命令行 GDB 兜底（不阻塞阶段 0） |
| 内置终端无法执行命令 | 切 `Craft` 让 AI 重跑单条命令定位，回传错误文本，不要跳过环境校验 |
| 原理图 PDF 打不开 | 回到阶段 0 输入项，确认文件名 `红牛开发板电路图(RedBull Schematic).pdf` 且可读；本阶段不解析 |

### 7.2 本机实测新增的回退（本次已实际执行）

| 现象 | 实测原因 | 已执行的回退动作 |
| --- | --- | --- |
| `winget install Kitware.CMake / Ninja-build.Ninja` 报 `InternetOpenUrl() failed. 0x80072efd` | 下载源 `github.com` HTTPS 超时（TCP 443 通、DNS 正常、无代理；`raw.githubusercontent.com` 同样超时） | 改用非 GitHub 源：`cmake.org` 官方 zip 与 Chocolatey `cmake.portable`；`ninja` 用 Chocolatey 包内嵌 `ninja-win_x32.zip` |
| `winget install ... --scope user` 报 `No applicable installer found` | NSIS 安装器不支持 user scope（实测 `Arm.GnuArmEmbeddedToolchain`、`NordicSemiconductor.JLink` 均为 nullsoft） | 去掉 `--scope user` 重试 → Arm GCC `Successfully installed` |
| `NordicSemiconductor.JLink` 报 `Installer failed with exit code: 2` | 安装器返回非 0（账面失败） | **不盲信退出码**：实测 `C:\Program Files\SEGGER\JLink_V818\` 下 `JLinkGDBServerCL.exe`、`JLink.exe`、`JLinkGDBServer.exe` 等齐全 → 按文档第 167 行记录绝对路径，不依赖 winget 登记与 PATH |
| `cmake.org` zip 两次 `curl` exit 28（13 MB / 21 MB，共 54,400,562 字节） | 该站点链路速率不足；但服务器支持 `Accept-Ranges: bytes` | 换用 Chocolatey CDN 的 `cmake.portable`（103,529,369 字节一次下载完成），二次解压得 x86_64 `cmake.exe`；`cmake.org` 的部分文件保留，可随时用 `-C -` 续传取稳定版 |
| PyPI、`raw.githubusercontent.com`、msstore 均无可用 ninja | GitHub 系与 PyPI 链路超时；msstore 搜索结果无关 | 用 `https://community.chocolatey.org/api/v2/package/ninja`（299,513 字节，HTTP 200）取内嵌 zip |
| SVD 缺失 | 未装 CubeMX / Keil MDK | 从 `https://www.keil.com/pack/index.idx` 读到 `Keil.STM32F1xx_DFP 2.4.1`，下载对应 `.pack`（50,237,740 字节，一次完成），解压 `SVD/` 目录 |

### 7.3 可复用的补齐命令（AI 生成，用户只需复制运行）

```powershell
# A. 若需稳定版 CMake 4.4.4（cmake.org 已下部分、支持断点续传）
& "$env:SystemRoot\System32\curl.exe" -fL -C - --connect-timeout 15 --max-time 300 -o "C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\cmake-4.4.4-windows-x86_64.zip" "https://cmake.org/files/v4.4/cmake-4.4.4-windows-x86_64.zip"

# B. 若 .svd 丢失，重新拉 Keil Pack（实测可达）
& "$env:SystemRoot\System32\curl.exe" -fL --connect-timeout 15 --max-time 300 -o "C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1.pack" "https://www.keil.com/pack/Keil.STM32F1xx_DFP.2.4.1.pack"

# B2. 解压并定位 SVD（.pack 是 zip，需先复制为 .zip 扩展名）
$p = "C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1.pack"; $z = "C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil_DFP.zip"; $d = "C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1"; Copy-Item $p $z -Force; Expand-Archive -Path $z -DestinationPath $d -Force; Get-ChildItem $d -Recurse -Filter "STM32F103*.svd" | Select-Object -ExpandProperty FullName

# C. winget 正确参数（实测自 winget install --help，勿凭记忆改）：--id / --exact / --source winget
#    / --silent / --accept-package-agreements / --accept-source-agreements / --disable-interactivity
#    NSIS 类包（Arm GNU Toolchain、J-Link）不要加 --scope user
winget install --id Arm.GnuArmEmbeddedToolchain --exact --source winget --silent --accept-package-agreements --accept-source-agreements --disable-interactivity

# D. msstore 源协议阻塞时的统一规避：所有 winget 命令加 --source winget
winget search --name ninja --source winget --disable-interactivity
```

其它情形：**PATH 未生效** → 直接运行第 1 节一键脚本（自动会话级补齐），或执行第 9 节持久化；**权限不足** → 优先用户级目录可移植包，不申请管理员权限；**网络整体不通** → 保留已下载的部分文件，用 `-C -` 续传，不做盲目重试；**任一 `MUST FAIL`** → 回退后重跑一键命令，未回到 `MUST_FAIL=0` 前不得进入阶段 1。

---

## 8. 本机实测记录

### 8.1 安装前基线（只读探测，逐字要点）

```
MISSING cmake / ninja / arm-none-eabi-gcc / arm-none-eabi-gdb / JLinkGDBServerCL / STM32CubeMX
FOUND   code => C:\Users\HenrryBian\AppData\Local\Programs\Microsoft VS Code\bin\code.cmd   (1.145.2, 9 extensions, no cortex-debug)
winget  => C:\Users\HenrryBian\AppData\Local\Microsoft\WindowsApps\winget.exe  (v1.29.380)
SEGGER / STMicroelectronics / Keil / Arm Pack 目录均不存在；PATH 中无 arm|gcc|cmake|ninja|SEGGER|STMicro 条目
SVD 限定目录递归搜索 STM32F103*.svd 无结果
原理图 PDF：114821 字节，OpenRead 成功
CodeBuddy CN 进程：D:\Program Files\CodeBuddy CN\CodeBuddy CN.exe
```

> **口径说明**：`code --list-extensions` 探测的是 **VS Code** 扩展，与文档要求的「CodeBuddy 插件市场能否搜到 `Cortex-Debug`」不是同一对象，不作为该结论依据；正确依据见 2.3 的 `.codebuddycn\extensions` 实测。

### 8.2 安装动作与失败回退时间线

1. `winget install --help` 实测参数；`winget show` 实测 4 个包的安装器类型与发布者。
2. `Kitware.CMake`、`Ninja-build.Ninja` + `--scope user` → `InternetOpenUrl() failed. 0x80072efd`。
3. 只读诊断：无代理、DNS 正常、`github.com` HTTPS 超时但 TCP 443 通；`developer.arm.com` / `files.nordicsemi.com` / `cmake.org` / `ninja-build.org` / `keil.com` / `st.com` 可达；PyPI、`raw.githubusercontent.com` 超时。
4. `Arm.GnuArmEmbeddedToolchain`、`NordicSemiconductor.JLink` + `--scope user` → `No applicable installer found`；去掉 `--scope user` 重试 → Arm GCC **Successfully installed**，J-Link 报 `exit code: 2`。
5. 实测 `C:\Program Files\SEGGER\JLink_V818\` 工具齐全 → 按绝对路径判定 J-Link 就绪。
6. Keil `index.idx` → `Keil.STM32F1xx_DFP 2.4.1`；Pack 完整下载 → `SVD\STM32F103xx.svd`。
7. Chocolatey `ninja` 包 → 内嵌 zip 二次解压 → `ninja.exe` `1.13.2`。
8. Chocolatey `cmake.portable` → 内嵌 `cmake-4.5.0-rc3-windows-x86_64.zip` 二次解压 → `cmake version 4.5.0-rc3`。
9. 运行一键脚本 → `MUST_PASS=9 MUST_FAIL=0`。

### 8.3 安装后最终清单（全部为实测绝对路径）

| 项 | 实测值 |
| --- | --- |
| `cmake` | `C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\cmake_x64\cmake-4.5.0-rc3-windows-x86_64\bin\cmake.exe` → `4.5.0-rc3` |
| `ninja` | `C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\ninja\bin\ninja.exe` → `1.13.2` |
| `gcc` / `gdb` / `size` | `C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\` → `14.2.1` / `15.2.90.20241130-git` / `2.43.1.20241119` |
| `JLinkGDBServerCL.exe` | `C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe`（590288 字节） |
| SVD | `C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd`（867312 字节） |
| CodeBuddy IDE | `D:\Program Files\CodeBuddy CN\CodeBuddy CN.exe` |
| `Cortex-Debug` | `C:\Users\HenrryBian\.codebuddycn\extensions\marus25.cortex-debug-1.13.0-pre6-universal`（`1.13.0-pre6`，依赖 4/4） |
| 原理图 PDF | 工作区根目录，114821 字节 |
| `STM32CubeMX` | 未安装（可选项） |

### 8.4 差异与风险提示（不得臆造，按实测记录）

- **CMake 是 `4.5.0-rc3` 预发布版**，来自 Chocolatey `cmake.portable`；winget 源里的稳定版是 `4.4.4`。本阶段只记录版本，不生成 `CMakeLists.txt`。**阶段 2 需注意**：CMake 4.x 对 `cmake_minimum_required` 最低版本要求较严，生成工程时应使用兼容的最低版本声明，并在阶段 2 验收时用实际 `cmake --version` 复核。若需稳定版，执行 7.3-A 的续传命令。
- **SVD 实际文件名是 `STM32F103xx.svd`**，与文档 0.1 占位符 `{SVD}` 写的 `STM32F103.svd` 不同。阶段 5 的 `svdFile` 必须填 8.3 的实测绝对路径，不要按占位符拼路径。
- **J-Link 实际目录是 `C:\Program Files\SEGGER\JLink_V818\`**，不是文档示例中出现的 `C:\Program Files\SEGGER\JLink\`；且分发者是 Nordic Semiconductor（`J-Link software and documentation pack setup`），winget 账面返回 exit 2。按文档第 167 行用绝对路径，不依赖 PATH、不依赖 winget 登记。
- **`cmake`/`ninja` 为可移植解压包**，不在持久 PATH；一键脚本自动做会话级补齐。裸跑 `--version` 需先执行第 9 节命令。
- 本阶段未执行 `JLinkGDBServerCL.exe --help`；J-Link 参数须在阶段 5 先探测 `--help` 再确定（文档第 559 行）。

---

## 9. 可选：把工具目录持久化到用户级 PATH（阶段 2 前建议执行）

> 默认**不执行**：本阶段按「仅当前会话 `$env:PATH`、不改机器级环境变量、保证可回退」执行。若要让任意新终端裸跑 `cmake --version`/`ninja --version`，复制运行下面这段（AI 生成，非手写代码）。**先记录 `OLD` 输出以便回退**。

```powershell
$add = @(
    'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\cmake_x64\cmake-4.5.0-rc3-windows-x86_64\bin',
    'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\ninja\bin',
    'C:\Program Files\SEGGER\JLink_V818'
)
$old = [Environment]::GetEnvironmentVariable('Path', 'User')
"OLD_USER_PATH=$old"
$parts = $old -split ';' | Where-Object { $_ -ne '' }
foreach ($p in $add) { if ($parts -notcontains $p) { $parts += $p } }
[Environment]::SetEnvironmentVariable('Path', ($parts -join ';'), 'User')
$env:PATH = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' +
            [Environment]::GetEnvironmentVariable('Path', 'User')
cmake --version
ninja --version
```

**回退**：把上面 `OLD_USER_PATH=` 之后的值原样写回，再把当前会话 PATH 重新拼接一次。

```powershell
[Environment]::SetEnvironmentVariable('Path', '<粘贴 OLD_USER_PATH 的值>', 'User')
$env:PATH = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' +
            [Environment]::GetEnvironmentVariable('Path', 'User')
```

---

## 10. AI 就绪判定（本次）

> **必须项全部就绪。**

依据：一键校验脚本 `MUST_PASS=9 MUST_FAIL=0`、`MISSING_ITEMS=none`；四条 `--version` 均返回有效版本号；`JLinkGDBServerCL.exe` 与 `STM32F103xx.svd` 均已定位到绝对路径；CodeBuddy IDE 的 `Plan`/`Craft`/内置终端/`@` 在本次会话实证可用；`Cortex-Debug 1.13.0-pre6` 已安装且依赖 4/4 → **阶段 5 走图形化单步主选**，命令行 GDB 保留为兜底；原理图 PDF 可读且未被解析。

**可选缺项（不阻塞阶段 0）**：`STM32CubeMX` 未安装（winget 源无匹配包）。按文档第 327、339 行 `.ioc` 为可选，CubeMX 软件本身不阻塞；示波器/逻辑分析仪可用 J-Link + 变量观察 + 日志替代。

**但有一个必须补齐的衔接项 —— STM32F1 HAL 驱动源**：文档第 291 行把「阶段 0 已就绪的 … STM32F1 HAL 与 CMSIS Pack」列为阶段 2 输入。本次实测已下载的 `Keil.STM32F1xx_DFP.2.4.1` 内 `stm32f1xx_hal_*.c` 计数为 **0**（只有 `STM32F103xx.svd`、`stm32f10x.h`、`system_stm32f10x.c`、`startup_stm32f10x_*.s` 与 StdPeriph 库），StdPeriph 与 Cube HAL 是两套不同 API，不能替代。**阶段 0 判定不受影响**（验收标准不含 HAL/CubeMX），但阶段 2 不能仅凭本报告直接开工，须先二选一：① 安装 STM32CubeMX，由它生成完整 HAL 驱动集 + `.ioc`；② 不装 CubeMX，由 AI 从 ST 官方 STM32CubeF1 固件包 / CMSIS 渠道取得 `stm32f1xx_hal_*.c` 等设备文件后生成初始化代码。具体来源与路径须在阶段 2 前实测，禁止凭记忆写。

**已人工确认（2026-11-11）**：`CON1` DB9 **配 RS232-USB 线**，与板上 `U5 ST3232` 的 RS232 电平匹配 —— 对应阶段 1 复核清单第 7 项，串口电平陷阱已排除。USB-TTL 直连 `CON1`（RS232 电平）会损坏 TTL 串口/MCU；若改用 USB-TTL，必须从扩展排针取 `PA9/PA10` 并共地。

**仍需用户人工确认（属于阶段 1）**：`JP5`/`JP6`/`JP8`/`JP9` 跳线、`JP6` 是否设为 SWD 可用模式、`JP8 BOOT0=0`、`J5` 的 `Vref`=3V3 与 GND 共地、板卡版本一致性、蜂鸣器有源/无源与有效电平；以及扩展面板中 `Cortex-Debug` 显示「已启用」的界面截图。

**阶段 1 准入建议**：通过。请按文档「一个阶段一个 Plan」新建独立计划，用 `@` 引用本报告与 `红牛开发板电路图(RedBull Schematic).pdf`，进入原理图解析与硬件引脚映射表锁定。届时才开始人工复核跳线与电平。阶段 0 验收期间未连接任何硬件；用户后续自行连接硬件的只读预检与人工确认见 10.1，不改变阶段 0 判定。

### 10.1 后续硬件连接预检与人工确认（2026-11-11，不改变阶段 0 判定）

| 项 | 结论 | 依据 |
| --- | --- | --- |
| USB 串口 | ✅ `USB Serial Port (COM3)`，FTDI `VID_0403/PID_6001`，状态 `OK`；`Open()`/`Close()` 成功（未收发数据） | 只读 PnP 枚举 + 端口探测 |
| Debug 串口接线 | ✅ 用户确认 `CON1` DB9 配 **RS232-USB 线** | 用户回传，阶段 1 复核清单第 7 项 |
| J-Link USB 枚举 | ✅ `J-Link driver`，`USB\VID_1366&PID_0101\001245593416`，`HasProblem=False`；软件 `V8.18` 可启动；照片中 LED 常亮、20pin 排线已接、板卡已上电 | PnP 属性 + 启动横幅 + 用户提供照片 |
| **J-Link 参数写法** | ✅ **已实测生效**：`-device STM32F103ZE -if SWD -speed 4000 -port 2331` | GDB Server 回显 `Target device: STM32F103ZE` / `Target interface: SWD` / `speed: 4000kHz`，无 unknown 参数警告。帮助参数 `-?`/`--help`/`-h`/`-help`/`/?`/`-Help` 全部无效、随包无参数手册，故改用「试探执行 + 观察设置回显」取得实证，未臆造 |
| **J-Link 连接结果** | ❌ **失败：连调试器本体即失败，尚未到目标芯片** | `Connecting to J-Link failed. Connected correctly?` / `Could not connect to J-Link. Please check power, connection and settings.`，连续两次复现 |
| 根因（推断，待验证） | 驱动层版本不匹配 | 设备绑定 `oem96.inf`（`jlink.inf`，Segger，`2.70.8.0`，2018-08-02）；驱动存储另有 `jlinkwinusb.inf`（WinUSB，2019）、`jlinkcdc.inf`；系统 `C:\Windows\System32\drivers\JLinkx64.sys` 为 88144 字节，V8.18 自带同名文件为 71400 字节；winget 安装 `NordicSemiconductor.JLink` 返回 `exit code: 2`，驱动安装步骤很可能未完成 |
| `JP6` 模式 | ⚠️ 用户确认当前为 **JTAG 模式**，与本项目 **SWD 单步调试** 目标冲突，需切到 SWD 可用模式 | 文档阶段 1 复核清单第 2 项 |
| 串口日志 | ⏳ 阶段 3 前无固件，`USART1` 无输出 | 本阶段未生成固件代码 |

> **安全边界**：改 `JP6` 跳线、插拔 `J5` 20pin 连接器前**必须先给板卡断电**（硬件禁忌第 1、10 条）。当前板卡已上电，请勿带电改动跳线；且当前瓶颈在 J-Link USB 驱动层，与跳线无关，须先修驱动、再断电切 SWD。（该瓶颈结论为**旧调试器**当时的状态，已被 10.2 的更换调试器实测结果取代。）
> 本轮仅做只读探测与一次连接试探（GDB Server 自行退出或超时终止），未烧录、未改跳线、未写 option bytes、未向目标芯片发起成功的 SWD 事务、未生成固件代码；已确认无残留 J-Link 进程。

### 10.2 更换 J-Link 后重跑检测（2026-11-11，不改变阶段 0 判定）

> 用户更换了另一台 J-Link 后要求重跑 10.1 的检测。**10.1 记录的是旧调试器的历史状态**，本节为新调试器的实测结果；阶段 0 判定不变。

| 项 | 结论 | 依据 |
| --- | --- | --- |
| USB 串口 | ✅ `COM3` 仍在，FTDI `VID_0403&PID_6001` 状态 `OK`；`Open()`/`Close()` 成功（未收发数据） | 只读 PnP 枚举 + 端口探测 |
| J-Link USB 枚举 | ✅ 新 `J-Link driver`，`USB\VID_1366&PID_0101\000099999994`，状态 `OK`；无 `Unknown USB Device`、无问题设备 | 只读 PnP 枚举（连接前后两次一致） |
| 调试器固件与硬件 | `Firmware: J-Link ARM V8 compiled Nov 28 2014 13:44:46`、`Hardware: V8.00`、`S/N: 99999994`、`Feature(s): RDI,FlashDL,FlashBP,JFlash,GDBFULL` | GDB Server 连接回显 |
| 参数写法 | ✅ 仍生效：`-device STM32F103ZE -if SWD -speed 4000 -port 2331`；回显 `Target device: STM32F103ZE` / `Target interface: SWD` / `speed: 4000kHz` | GDB Server 设置回显 |
| **调试器本体连接** | ✅ **`J-Link is connected.`** —— 相比 10.1「连调试器本体即失败」已恢复 | GDB Server stdout |
| **目标芯片 SWD 连接** | ❌ **失败，瓶颈前移到目标侧**：`Checking target voltage...` 之后 `Connecting to target failed. Connected correctly?` / `Could not connect to target. Please check power, connection and settings.` | GDB Server stdout/stderr |
| 速率交叉验证 | `-speed 4000` 与 `-speed 100` 两次失败点完全相同 → **排除 SWD 速率因素** | 两次完整日志 |
| 固件升级 | ⏳ 本轮**未执行、未写入任何固件**；两次 GDB Server 均自行 `Shutting down...` 退出，未强杀任何升级窗口 | 进程与日志 |

**结论**：USB 枚举、驱动、参数、速率侧均已排除，剩余故障域为**目标侧 SWD 物理链路**。按优先级待人工确认：

1. 20pin 排线（`J5`）是否已接回目标板、方向是否正确 —— 此前排查 USB 识别时曾建议拔掉排线做最小连接测试，可能尚未接回。
2. 目标板是否上电；J-Link `Vref`（pin 1）是否为 3V3；GND 是否共地。
3. `JP6` 是否已切到 **SWD** 可用模式，且切换时**先断电**；`JP8 BOOT0=0`。
4. `SWDIO` / `SWCLK` / `nRESET` 是否与 20pin 对应、接触是否良好。

> **固件风险提示**：新调试器 `S/N: 99999994` + `Hardware V8.00` + 2014 年固件，是 V8 兼容/克隆类调试器的典型特征，且与用户先前拍摄的背面标签（`Hardware V8.0`、`S/N:150916796`）序列号不一致。SEGGER V8.18（`JLinkARM.dll V8.18`，DLL 编译 2025-03-05）可能对其提示固件升级；**不要盲刷官方固件**，有变砖风险。用户此前的「升级 FW」授权针对旧设备，换新设备后不自动沿用，需重新确认。
>
> **安全边界**：改 `JP6`、插拔 `J5` 20pin 前必须先给板卡断电。本轮未改驱动、未改跳线、未烧录、未写 option bytes、未生成固件代码；两次 GDB Server 均自行退出。检测后系统存在 `JLink`(PID 23512)、`JLinkGUIServer`(PID 11096) 与标题为 `J-Link Commander V8.18` 的终端窗口，未见固件升级弹窗；按用户要求**未强制终止**任何 J-Link 进程，如需干净环境请手动关闭该 Commander 窗口。

### 10.3 硬件接线修复后的闭环验证（2026-11-11，不改变阶段 0 判定）

> 用户按 10.2 的待确认项完成接线与跳线设置后，在 `SEGGER J-Flash V8.18` 中确认连接成功。**10.2 中「目标芯片 SWD 连接失败」为接线修复前的历史状态**，本节为修复后结果。

| 项 | 结论 | 依据 |
| --- | --- | --- |
| 连接结果 | ✅ `Connected successfully` / `Initialized successfully` | 用户提供的 J-Flash 日志截图 |
| 接口与速率 | `TIF Type: SWD`、`Init. speed` / `Speed`: `4000 kHz`、`Target interface speed: 4000 kHz (Fixed)`、`Host connection: USB [Device 0]` | J-Flash `Project information` |
| 调试端口枚举 | `DPv0 detected`、`CoreSight SoC-400 or earlier`、`AP[0]: AHB-AP (IDR: 0x14770011)`、`Core found`、`AHB-AP ROM base: 0xE00FF000` | J-Flash Log |
| 内核识别 | `CPUID register: 0x411FC231`（Implementer `0x41` = ARM）、`Found Cortex-M3 r1p1, Little endian`、`FPUnit: 6 code (BP) slots and 2 literal slots`、`Core ID: 0x1BA01477` | J-Flash Log |
| CoreSight 组件 | `ROMTbl[0] @ 0xE00FF000`，`SCS` / `DWT` / `FPB` / `ITM` / `TPIU-Lite` / `ETM-M3` 全部枚举 | J-Flash Log |
| 10.2 失败归因 | 属目标侧物理链路问题（20pin 排线未接 / `Vref` 未接 / GND 未共地 / `JP6` 模式），非 USB 驱动、GDB Server 参数或 SWD 速率问题；按 10.2 清单接线后消除 | 10.2 两次日志 + 本节成功日志 |

**边界说明（不等于阶段 5 完成）**：J-Flash 当前是**未选定具体器件**的通用连接 —— `Project name: ---`、`CPU: Cortex-M3`（非 `STM32F103ZE`）、`Use target RAM: 32 KB @ 0x0`（J-Flash 通用默认值，不是板卡真实 RAM 配置）、`Flash bank 0: (disabled)`（未加载 Flash 算法）、`Check core ID: No`。`CPUID 0x411FC231` 只证明内核为 Cortex-M3 r1p1，不能单独证明具体 ST 型号。因此：

1. 本节证明 **SWD 物理链路 + 调试端口 + Cortex-M3 内核** 已连通，阶段 1 复核清单第 8 项「J-Link 接口匹配」获得实证（`Vref` / 共地可被连接成功间接佐证）。
2. 阶段 5 仍需显式 `-device STM32F103ZE`、加载对应 Flash 算法、使用 SVD 绝对路径 `Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd`，并确认 `JP8 BOOT0=0`；在此之前不能烧录。
3. 日志中的 `Found 1 JTAG device` 是 J-Link 通用措辞，实际接口以 `TIF Type: SWD` 为准，不代表 `JP6` 处于 JTAG 模式。

> **操作边界**：用户当前 `JFlash`(PID 22756，标题 `SEGGER J-Flash V8.18 - [ *]`) 仍连接目标；为避免与其争用 J-Link 句柄，本轮**未启动** `JLinkGDBServerCL` 做独立复核，仅做图片判读与只读进程/枚举查询；未烧录、未改跳线、未改驱动、未写固件。待用户关闭 J-Flash 后，再用 `-device STM32F103ZE -if SWD -speed 4000 -port 2331` 补一次项目工具链的独立连接证据。

### 10.4 J-Link 连接功能检测收尾与阶段完成判定（2026-11-11）

> 用户要求把 J-Link 连接功能检测的最终结论更新进记录，并据此结束本阶段。本节汇总 `10.1`–`10.3` 的证据链，给出收尾判定。

| 检测项 | 最终结论 | 证据 |
| --- | --- | --- |
| USB 设备枚举 | ✅ `J-Link driver`，`USB\VID_1366&PID_0101\000099999994`，状态 `OK`；无未知设备 / 无问题设备 | 10.2 + 10.4 本轮只读 PnP 复查 |
| 调试器本体 | ✅ `J-Link is connected.`（`S/N 99999994`、`Hardware V8.00`、FW `2014-11-28`） | 10.2 GDB Server 日志 |
| GDB Server 参数 | ✅ `-device STM32F103ZE -if SWD -speed 4000 -port 2331` 实测生效；`-speed 100` 交叉验证排除速率因素 | 10.1 / 10.2 设置回显 |
| SWD 目标连接 | ✅ `Connected successfully`（J-Flash V8.18，`SWD 4000 kHz`）；`DPv0` / `AHB-AP` / `CPUID 0x411FC231` / `Cortex-M3 r1p1` / `Core ID 0x1BA01477` 全部枚举 | 10.3 J-Flash 日志截图 |
| 串口 | ✅ `COM3`，FTDI `VID_0403/PID_6001`，`Open()`/`Close()` 成功；`CON1` 配 RS232-USB 线 | 10.1 / 10.2 |
| 固件升级 | ⏳ **本阶段不执行、未写入任何固件**；记为后续事项，需重新确认后再评估 | 10.2 / 10.3 风险提示 |

**J-Link 连接功能检测判定：通过。** 故障链已完整闭环：10.1 的 USB/驱动层问题 → 换 J-Link 后 10.2 收敛到目标侧物理链路 → 按清单接线后 10.3 成功识别 Cortex-M3 内核。

**阶段完成判定**：以 `10.1`–`10.4` 为范围的 **硬件连接预检与 J-Link 连接功能检测阶段** 判定 **完成**。此判定不改变阶段 0 验收结论（`MUST_PASS=9 MUST_FAIL=0`），也不代替工作流中阶段 1 的正式验收。

**未纳入本阶段、需带入后续阶段的事项**：

1. 阶段 1 正式验收仍需独立 Plan，完成原理图解析与引脚映射表锁定，以及 `JP5` / `JP6` / `JP8` / `JP9` 跳线、板卡版本一致性、蜂鸣器有源/无源与有效电平、`Cortex-Debug` 启用截图等人工复核。
2. 阶段 5 前需显式 `-device STM32F103ZE` + 加载 Flash 算法 + SVD 绝对路径；本阶段 J-Flash 为未选器件的通用 Cortex-M3 连接，**不能烧录**。
3. 阶段 2 前必须补齐 `stm32f1xx_hal_*.c` 等 STM32F1 HAL 驱动源（CubeMX 软件可选，HAL 源不可缺）。
4. **J-Link FW 升级（留待后续）**：暂不执行。该调试器为 V8 兼容/克隆类（`S/N 99999994`），SEGGER V8.18 可能提示升级，盲刷有变砖风险；如确需升级，须先核对设备型号/序列号与官方流程、重新确认后再单独进行，不随本阶段收尾执行。
5. 可选：关闭 `JFlash`(PID 22756) 后，用 `-device STM32F103ZE -if SWD -speed 4000 -port 2331` 补一次项目工具链（`JLinkGDBServerCL`）的独立连接证据；该项**不阻塞**本阶段完成判定。

> **本轮操作边界**：`JFlash` 仍连接目标，为避免争用 J-Link 句柄，本轮仅做只读进程 / PnP 复查与文档记录更新；未启动 GDB Server、未烧录、未改跳线、未改驱动、未写固件、未终止任何 J-Link 进程。

---

## 11. 一键脚本本次完整输出（可整段回传）

```
=== STAGE0 ENV CHECK (read-only, offline, idempotent) ===
WORKSPACE=C:\Users\HenrryBian\CodeBuddy\20261110233826
INFO SESSION_PATH_AUGMENTED dirs=4 | current process only, no persistent change
MUST PASS cmake | version=cmake version 4.5.0-rc3 path=C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\cmake_x64\cmake-4.5.0-rc3-windows-x86_64\bin\cmake.exe
MUST PASS ninja | version=1.13.2 path=C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\ninja\bin\ninja.exe
MUST PASS arm-none-eabi-gcc | version=arm-none-eabi-gcc.exe (Arm GNU Toolchain 14.2.Rel1 (Build arm-14.52)) 14.2.1 20241119 path=C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-gcc.exe
MUST PASS arm-none-eabi-gdb | version=GNU gdb (Arm GNU Toolchain 14.2.Rel1 (Build arm-14.52)) 15.2.90.20241130-git path=C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-gdb.exe
OPTIONAL PASS arm-none-eabi-size | version=GNU size (Arm GNU Toolchain 14.2.Rel1 (Build arm-14.52)) 2.43.1.20241119
MUST PASS JLinkGDBServerCL | path=C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe bytes=590288 | file probed only, NOT executed
INFO JLINK_GDB_SERVER_ABS=C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe
MUST PASS STM32F1_SVD | path=C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd bytes=867312
INFO SVD_ABS=C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD\STM32F103xx.svd
MUST PASS CodeBuddy_IDE | path=D:\Program Files\CodeBuddy CN\CodeBuddy CN.exe
MUST PASS Cortex-Debug_ext | dir=marus25.cortex-debug-1.13.0-pre6-universal version=1.13.0-pre6 deps=4/4 | primary path available for stage 5
MUST PASS Schematic_PDF | bytes=114821 openread=True | NOT parsed in stage 0
OPTIONAL MISSING STM32CubeMX | no winget package; .ioc is optional in stage 2; install from ST official package if needed
OPTIONAL PASS winget | path=C:\Users\HenrryBian\AppData\Local\Microsoft\WindowsApps\winget.exe version=v1.29.380
OPTIONAL PASS VSCode_CLI | path=C:\Users\HenrryBian\AppData\Local\Programs\Microsoft VS Code\bin\code.cmd | reference only, dev front end is CodeBuddy IDE
INFO CODEBUDDY_UI | Plan/Craft/Ask, built-in terminal, @ context: session-verified; see docs/env_check.md
INFO HARDWARE | oscilloscope/logic analyzer optional, not probed by script; J-Link + variable/log observation is the substitute
INFO SWD_SAFETY | stage 0 installs software only: no board connected, no power, no flashing, no jumper change, no option bytes, no read protection
INFO NO_FIRMWARE | stage 0 generated no .c/.h/.s/.ld, no CMakeLists.txt, no .ioc, no debug config
=== SUMMARY ===
MUST_PASS=9 MUST_FAIL=0
MISSING_ITEMS=none
VERDICT_HINT=ALL_MUST_PASS
NOTE=AI issues the final stage-0 verdict and failure fallback per docs/env_check.md acceptance gate
=== END ===
```
