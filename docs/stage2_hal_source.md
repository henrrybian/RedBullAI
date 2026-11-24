# 阶段 2：官方 HAL / CMSIS 来源实测与取回证据（唯一证据文件）

> 项目：STM32F103ZET6 红牛开发板，蜂鸣器 PWM，`USART1` Debug 输出，J-Link SWD 调试。
> 生成日期：2026-11-12 ｜ 平台：Windows + PowerShell ｜ 阶段：2 · `probe-hal-sources`
> 依据：`@.codebuddy/plans/阶段2-HAL初始化与工程骨架生成_d8bd792e.md`、`@.codebuddy/plans/阶段0-资料与环境准备-环境校验与就绪判定_26b1905e.md`、`@.codebuddy/plans/阶段1-原理图解析与PB2寄存器实测_aff9afe7.md`、`@docs/hardware_pins.md`（锁定版）、`@docs/env_check.md`（第 145/339/431 行的 HAL 源缺口）。
> **本文件是 HAL 来源决策的唯一证据文件**。下文全部路径、版本、commit、文件计数、字节数与哈希均来自本机实测输出，未凭记忆填写。

---

## 1. 结论（硬阻塞已解除）

- 阶段 0 记录的唯一阶段 2 硬阻塞是 STM32F1 **Cube HAL 驱动源缺失**（`stm32f1xx_hal_*.c` = 0，`@docs/env_check.md` 145/339/431 行）。本次按第 339 行「二选一」之 **② 不装 CubeMX，由 AI 从 ST 官方 STM32CubeF1 / CMSIS 渠道取得** 解决。
- 实测取回 **ST 官方 GitHub 三个仓库** 的文件，精确复制 **111 个文件 / 6,068,581 字节** 到工作区（逐文件 SHA256 见第 7 节），构成 `Drivers/STM32F1xx_HAL_Driver/`、`Drivers/CMSIS/`，以及 `Core/Src/system_stm32f1xx.c`、`startup_stm32f103xe.s`、`STM32F103ZETx_FLASH.ld`。
- 未安装 STM32CubeMX；**未用 StdPeriph 替代 Cube HAL**；`Keil.STM32F1xx_DFP.2.4.1` 仅含 `STM32F103xx.svd`、`stm32f10x.h`、`system_stm32f10x.c`、`startup_stm32f10x_*.s` 与 StdPeriph 库，只作缺口对照，不参与本工程编译。
- 未修改任何只读输入：`@docs/hardware_pins.md`（锁定版）、`@docs/env_check.md`、`STM32_Buzzer_PWM_Workflow.md`、两个 PDF、`scripts/env_check.ps1`、`RedbullDemo.code-workspace` 均未改动。

---

## 2. 探测命令与实测结果（先探测，再决定获取方式）

### 2.1 本机已有资源探测

```powershell
$roots = @('C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools','C:\Users\HenrryBian\STM32Cube',
           'C:\Users\HenrryBian\Downloads','C:\Program Files\STMicroelectronics',
           'C:\Program Files (x86)\STMicroelectronics','C:\Keil_v5',
           'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1')
foreach ($r in $roots) { if (Test-Path -LiteralPath $r) { "ROOT_EXISTS $r" } else { "ROOT_MISSING $r" } }

Get-ChildItem -LiteralPath 'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools' -Recurse -File |
  Where-Object { $_.Name -match 'stm32f1xx_hal|core_cm3|stm32f1xx\.h|system_stm32f1xx|startup_stm32|\.ld$' } |
  Select-Object -First 80 -ExpandProperty FullName
```

实测输出要点：

| 项 | 结果 |
| --- | --- |
| 存在的根目录 | `EmbeddedTools`、`Downloads`、Arm GNU Toolchain `14.2 rel1` |
| 不存在的根目录 | `STM32Cube`、`C:\Program Files\STMicroelectronics`、`C:\Program Files (x86)\STMicroelectronics`、`C:\Keil_v5` |
| `EmbeddedTools` 内匹配文件 | 全部为 `Keil.STM32F1xx_DFP.2.4.1` 下的 `startup_stm32f10x_*.s`（StdPeriph/Keil 风格）；**无 `stm32f1xx_hal_*.c`、无 `core_cm3.h`、无 `stm32f1xx.h`、无 `.ld`** |
| 工作区内检索 | `@.codebuddy/plans/阶段2-…md` 第 100–123 行的只读复核结论：`*.c/*.h/*.s/*.ld/*.ioc` 均为 0 files |

### 2.2 网络与工具可达性探测

```powershell
Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/STMicroelectronics/STM32CubeF1/master/README.md' -Method Head -TimeoutSec 20
Invoke-WebRequest -Uri 'https://github.com/STMicroelectronics/STM32CubeF1' -Method Head -TimeoutSec 20
Invoke-WebRequest -Uri 'https://www.st.com/en/embedded-software/stm32cubef1.html' -Method Head -TimeoutSec 20
Get-Command git.exe, curl.exe
```

| 探测 | 实测结果 |
| --- | --- |
| `github.com` / `st.com` | HTTP 200 |
| `raw.githubusercontent.com` HEAD | 超时；改 GET 后 `README.md` 返回 200 / 5164 B |
| `git.exe` | `C:\Program Files\Git\cmd\git.exe` |
| `curl.exe` | `C:\Windows\system32\curl.exe` |
| GitHub API `releases/latest` | 404（该仓库无 GitHub Release，用 tags 定位版本） |
| GitHub API `tags?per_page=10` | 实测最新 tag **`v1.8.7`**，commit `d12e75247d5bcedc734f829b394517ab4c2726e3`；其后 `v1.8.6` … `v1.8.0` |
| GitHub API 仓库元数据 | `default_branch = master`，`pushed_at = 2026-09-16T14:27:28Z` |

### 2.3 官方包文件树探测（关键发现：HAL 是子模块）

```powershell
curl.exe -sS -L -H 'Accept: application/vnd.github+json' `
  'https://api.github.com/repos/STMicroelectronics/STM32CubeF1/contents/Drivers?ref=master'
curl.exe -sS -L 'https://raw.githubusercontent.com/STMicroelectronics/STM32CubeF1/master/.gitmodules'
```

实测：`Drivers/STM32F1xx_HAL_Driver` 与 `Drivers/CMSIS/Device/ST/STM32F1xx` 均为 **git submodule**（`type=file/submodule`），`.gitmodules` 给出官方仓库地址：

| 子模块路径 | 官方仓库 | CubeF1 master 锁定的 commit |
| --- | --- | --- |
| `Drivers/STM32F1xx_HAL_Driver` | `https://github.com/STMicroelectronics/stm32f1xx_hal_driver.git` | `bd368aa335c529641d2e567574885e02deacc64a` |
| `Drivers/CMSIS/Device/ST/STM32F1xx` | `https://github.com/STMicroelectronics/cmsis_device_f1.git` | `c8e9a4a4f16b6d2cb2a2083cbe5161025280fb22` |

`Drivers/CMSIS/Include`（CMSIS Core 头）位于 CubeF1 主仓，实测 21 个头文件；HAL 子模块 `Inc/` 实测 67 个头文件、`Src/` 实测含 `stm32f1xx_hal.c` 等 45 个 `stm32f1xx_hal_*.c`；CMSIS 设备子模块实测 `Include/stm32f1xx.h` / `stm32f103xe.h` / `system_stm32f1xx.h`、`Source/Templates/system_stm32f1xx.c`、`Source/Templates/gcc/startup_stm32f103xe.s`、`Source/Templates/gcc/linker/STM32F103XE_FLASH.ld`。

### 2.4 被排除的获取通道（实测失败，如实记录）

| 通道 | 命令要点 | 实测结果 |
| --- | --- | --- |
| codeload zipball 整包 | `curl.exe -L -o %TEMP%\STM32CubeF1-v1.8.7.zip https://codeload.github.com/.../zip/refs/tags/v1.8.7` | 300 s 超时，仅收到 11,714,665 B 的不完整包（HTTP 200 但传输中断），**未使用、已弃用** |
| `raw.githubusercontent.com` 大文件 | GET `Src/stm32f1xx_hal.c` / `stm32f1xx_hal_gpio.c` | `Recv failure: Connection was reset` / 40 s 超时，**未使用** |
| GitHub Contents API 逐文件 | `contents/Drivers/STM32F1xx_HAL_Driver/Src/...` | 404（子模块路径在主仓 API 中不可直接展开），且未认证 API 有 60 次/小时配额，**未作为批量通道** |

最终采用 **`git clone --depth 1`**（子模块仓库）+ **稀疏克隆**（主仓）一次取回，避免逐文件下载与配额限制。

---

## 3. 实际来源、版本与 commit（三仓库锁定）

| # | 官方来源 | 本机绝对路径（`$env:TEMP` 下的临时检出） | 实测 `git rev-parse HEAD` |
| --- | --- | --- | --- |
| 1 | `STMicroelectronics/STM32CubeF1`（主仓，`master`） | `C:\Users\HenrryBian\AppData\Local\Temp\stage2_probe\STM32CubeF1` | `c7656f1fee178058be2f8ebfc8df9eaf91dc565e` |
| 2 | `STMicroelectronics/stm32f1xx_hal_driver`（Cube HAL 子模块） | `C:\Users\HenrryBian\AppData\Local\Temp\stage2_probe\stm32f1xx_hal_driver` | `bd368aa335c529641d2e567574885e02deacc64a` |
| 3 | `STMicroelectronics/cmsis_device_f1`（CMSIS 设备子模块） | `C:\Users\HenrryBian\AppData\Local\Temp\stage2_probe\cmsis_device_f1` | `c8e9a4a4f16b6d2cb2a2083cbe5161025280fb22` |

版本证据（均从检出文件实测读取，非记忆）：

- 主仓 `Release_Notes.html` 实测首条记录：`V1.8.7 / 12-February-2026` → **STM32CubeF1 V1.8.7**。
- 主仓 `package.xml` 实测：`<PackDescription Release="FW.F1.1.8.0" Patch="FW.F1.1.8.7">`。
- 2、3 号仓库的 HEAD **与 CubeF1 master `.gitmodules` 子模块指针逐字一致**（`bd368aa…` / `c8e9a4a…`），即取回的正是官方包锁定的 HAL / CMSIS 版本，不是任意 master 快照的混搭。
- 说明：`v1.8.7` tag 的 commit 为 `d12e752…`（tags API 实测），主仓稀疏检出为 `master` HEAD `c7656f1…`；二者 release 版本一致（`V1.8.7`），本文按实际检出 commit 记录，不混写。

### 获取命令（复现用）

```powershell
$b = Join-Path $env:TEMP 'stage2_probe'
git clone --depth 1 https://github.com/STMicroelectronics/stm32f1xx_hal_driver.git (Join-Path $b 'stm32f1xx_hal_driver')
git clone --depth 1 https://github.com/STMicroelectronics/cmsis_device_f1.git    (Join-Path $b 'cmsis_device_f1')
git clone --depth 1 --filter=blob:none --sparse https://github.com/STMicroelectronics/STM32CubeF1.git (Join-Path $b 'STM32CubeF1')
git -C (Join-Path $b 'STM32CubeF1') sparse-checkout set Drivers/CMSIS/Include
```

---

## 4. 复制决策（最小必需集，非整包全量编译）

按阶段 2 计划「只复制最小必需 HAL 源集合」与 `@docs/hardware_pins.md` 锁定的外设需求（RCC / GPIO / USART1 / TIM 路径 A / SysTick）：

| 目标目录 | 来源 | 选取规则 | 文件数 |
| --- | --- | --- | --- |
| `Drivers/STM32F1xx_HAL_Driver/Inc/` | 仓库 2 `Inc/*.h` | **全量 67 个头 + Legacy 3 个兼容头**（含 `stm32f1xx_hal_conf_template.h`、`stm32_assert_template.h` 与全部 LL 头），避免模块头互相 include 缺文件 | 70 |
| `Drivers/STM32F1xx_HAL_Driver/Src/` | 仓库 2 `Src/` | **最小 14 个 `.c`**：`hal.c`、`rcc.c`、`rcc_ex.c`、`gpio.c`、`gpio_ex.c`、`cortex.c`、`dma.c`、`flash.c`、`flash_ex.c`、`pwr.c`、`exti.c`、`uart.c`、`tim.c`、`tim_ex.c`（覆盖 HAL_Init / SysTick / RCC 时钟 / GPIO / USART1 用 UART / TIM 路径 A 基础 / 依赖关系） | 14 |
| `Drivers/CMSIS/Include/` | 仓库 1 `Drivers/CMSIS/Include/*.h` | 全量 21 个 CMSIS Core 头（含 `core_cm3.h`、`cmsis_gcc.h`、`cmsis_version.h`、`cmsis_compiler.h`） | 21 |
| `Drivers/CMSIS/Device/ST/STM32F1xx/Include/` | 仓库 3 `Include/` | 仅 `stm32f1xx.h`、`stm32f103xe.h`、`system_stm32f1xx.h`（`STM32F103xE` 器件族） | 3 |
| `Core/Src/system_stm32f1xx.c` | 仓库 3 `Source/Templates/system_stm32f1xx.c` | 官方 CMSIS system 文件，原样复制 | 1 |
| `startup_stm32f103xe.s` | 仓库 3 `Source/Templates/gcc/startup_stm32f103xe.s` | 官方 GCC 启动文件与中断向量表，原样复制 | 1 |
| `STM32F103ZETx_FLASH.ld` | 仓库 3 `Source/Templates/gcc/linker/STM32F103XE_FLASH.ld` | 官方链接脚本，**仅改文件名**（`STM32F103XE_FLASH.ld` → `STM32F103ZETx_FLASH.ld`），内容原样：FLASH 512K @ `0x08000000`、RAM 64K @ `0x20000000`、`_estack=0x2000FFFF`、`ENTRY(Reset_Handler)` | 1 |
| **合计** | | | **111** |

未复制：其余 `stm32f1xx_hal_*.c`（ADC/CAN/I2C/SPI/ETH/USB/…）、`*_template.c`（除 conf 模板头）、BSP、Middlewares、Projects 示例；`Inc/Legacy/` 3 个兼容头已按构建验证结果补复制（见第 5 节）。这些文件不进入 CMake 源列表。

---

## 5. 复制与校验命令

```powershell
$ErrorActionPreference = 'Stop'
$root = 'c:\Users\HenrryBian\CodeBuddy\20261110233826'
# Add-Copy：建目录 → Copy-Item → Get-Item.Length → Get-FileHash -Algorithm SHA256
# 按第 4 节清单逐文件执行，输出 `相对路径\t字节数\tSHA256` 到 %TEMP%\stage2_manifest.tsv
```

实测输出：

```
HAL_CORE_EXISTS=True
COPIED=108 TOTAL_BYTES=5784126 MANIFEST=C:\Users\HENRRY~1\AppData\Local\Temp\stage2_manifest.tsv
```

**构建验证阶段补复制（实测）**：首次 `cmake --build build/Debug` 报错 `stm32f1xx_hal_def.h:30:10: fatal error: Legacy/stm32_hal_legacy.h: No such file or directory`；按官方子模块 `Inc/Legacy/` 实测补复制 3 个文件（合计 284455 字节，`COPIED=3 TOTAL_BYTES=284455`），随后重跑构建 0 error 通过。逐文件哈希已并入第 7 节。`Get-ChildItem` 递归计数）：
、`Drivers/CMSIS/` 24 个文件、`Core/Src/system_stm32f1xx.c` + 根目录 2 个文件，
。

---

## 6. 文件来源标注（满足提示词硬约束 4）

| 工作区文件 | 来源 | 生成方 |
| --- | --- | --- |
| `Drivers/STM32F1xx_HAL_Driver/**` | ST 官方 `stm32f1xx_hal_driver` @ `bd368aa…` | ST 官方（原样复制） |
| `Drivers/CMSIS/Include/**` | ST 官方 `STM32CubeF1` V1.8.7 主仓 @ `c7656f1…` | ST 官方（原样复制） |
| `Drivers/CMSIS/Device/ST/STM32F1xx/Include/**`、`Core/Src/system_stm32f1xx.c` | ST 官方 `cmsis_device_f1` @ `c8e9a4a…` | ST 官方（原样复制） |
| `startup_stm32f103xe.s` | 同上，`Source/Templates/gcc/startup_stm32f103xe.s` | ST 官方（原样复制） |
| `STM32F103ZETx_FLASH.ld` | 同上，`Source/Templates/gcc/linker/STM32F103XE_FLASH.ld` | ST 官方（原样复制，仅改名） |
| `Core/Inc/stm32f1xx_hal_conf.h` | 官方 `stm32f1xx_hal_conf_template.h` | **AI 生成**（按阶段 2 最小模块裁剪，见 `Core/Inc/` 文件头注释） |
| `Core/Inc/main.h`、`Core/Src/main.c`、`Core/Src/stm32f1xx_it.c`、`Core/Inc/stm32f1xx_it.h`、`CMakeLists.txt`、`cmake/gcc-arm-none-eabi.cmake` | 无现成官方模板可用 | **AI 生成**（阶段 2 `generate-stage2-skeleton` 产出） |

StdPeriph（`stm32f10x_*.h/.c`、`startup_stm32f10x_*.s`）**未进入本工程**，仅作为阶段 0 缺口对照。

## 7. 精确复制清单与逐文件 SHA256

工作区落地复核（ 为复制后工作区文件的实测值（Get-FileHash -Algorithm SHA256）。

| 工作区相对路径 | 字节数 | SHA256 |
| --- | ---: | --- |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal.h | 11448 | `701749189012D2BDA33A629F2BAF420B8A870B517B74EF43DC4311740BEB804A` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_adc.h | 53554 | `0C7C9C2D9124533192FF5033060D253ED401AD0A000D74ED5103C851C371FDC5` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_adc_ex.h | 47599 | `CBB46E52A3D1C3E572E9BC90A77544F24063FC3515A0F78EB1F1CAA9E2C58D9D` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_can.h | 44419 | `4A7A12196AB55917FB5789F9CCF0D89D7E386090B9750634B18FF79DE2063CA4` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_cec.h | 22931 | `29A22422D180F1EF10A1E7675535ADE5171930504B16560F28FA2B8BE6D2A411` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_conf_template.h | 16319 | `013644D093E4075A62951E33FAD28C551D43754DE543C04E89AAF5947DB2474C` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_cortex.h | 17477 | `EEDB42E4F4F197CEC250E18FAA1A32711019C72DDEC956FBC7596032F5228315` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_crc.h | 5093 | `C1B1A79C6F128CF431BDC803BF478D8413BE875771EF4F2D9DD3647C5FB844E6` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_dac.h | 16026 | `70D14234C3AF4F668825DD4C8CF100822EA45840C1687AE8641E3862623DAD38` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_dac_ex.h | 16324 | `179EF64F9AAD689B42C2982864DD101001F83446719CC53EBF61D739073C1A1A` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_def.h | 7636 | `DCC6DB49AEB887D662D15C1C32E4DCA10B76B40C33A84A1059D547159EBEAC4D` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_dma.h | 18209 | `34C7ACD9BFD2F72CD08DB6653484917C7CE6B5EC5EBC0D1D5F49C9ADE9148E50` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_dma_ex.h | 12664 | `231BBEBF3511E3AC40BABED7B42BC6C279A77B699BB28D1C57D53C4868214C41` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_eth.h | 105627 | `C91E59DD120550687A5127306816F625359212E52CE554EF803D00B22B655A9D` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_exti.h | 12132 | `0BC0C61BC55C7D4D698FFCF8D71A9FEBE5B1201BAACA0437C3C3DA1F62F449FF` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_flash.h | 9145 | `2507E53DAB0C8340E4D8189929A46F34D25B059BDAA639CF1C754F186F240245` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_flash_ex.h | 36796 | `97A07EF38D87CC332FD30011A68E02725E112163F13259210E0F4A7F39476E98` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_gpio.h | 11876 | `3AE8AC366A2A55952FC4BADA166E88F6D15FB884492048191F267F79FFA73D44` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_gpio_ex.h | 36048 | `769EB507539209EC99906CB586E60C760C96FFCEDE2754E5C43717404E3F18DE` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_hcd.h | 12581 | `C4281A57D5F462E637F72645B61B60E5826923C0342B9DBF872C8BAA03F2EC2C` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_i2c.h | 35641 | `FFC3F56F746113BAB72283E016999760DA95384D815E5E7DD8899CB551C0C84A` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_i2s.h | 23816 | `C5AF7244A12AF777E382A9ECD3757AEF872DCAE607EA1CDE65EEA2D6DBE045ED` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_irda.h | 31641 | `F2D57F0DE43DBFA20D8BB81CBA01F0A9A818B140B7B92D4E21A991B024BDC3C9` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_iwdg.h | 6827 | `7131D896539884C9FBAA2B746D1ACB0DBF7A9B0A677AB0C732BB9928B210E187` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_mmc.h | 37878 | `91401D101A44EE5236F34D3D231AD308F31438BFBDDB289D44220F79507AC913` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_nand.h | 14939 | `E8E7756156922F29FE4E76776806C58B9E43BC81C8F3A9DC5E2CAE1CF1C3F8C8` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_nor.h | 11611 | `AFC1FFEEBDBCE65F52AB2B9E9B016E0632802D4E5692E465181A8F9B45CF04BE` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_pccard.h | 11973 | `3429BD8F7ADC9395479EA2258D5DBFBC2A521156BEB733AD7011747204306207` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_pcd.h | 38598 | `680D0103BA7CAAC76CF58B1FF63002E77F12FEC5099232F0390D641A48D0265A` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_pcd_ex.h | 2568 | `ED48946B08F1BDB4D1A26BC419BFFDC8A2BD6D03A97923EE2635B71781BCEA42` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_pwr.h | 11918 | `65B32570744F35CEA1AFAA080709BF0B9EE1AE637BD9DC6F03647B235E8B91A7` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_rcc.h | 68078 | `4147955E90B93093BB55063898048A83EB19E0BCA4801C58C326D57F1BA6C8B5` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_rcc_ex.h | 101206 | `FA0D67A0BF02D81DF5F6B96247D30AEE26BC97EBF4D6B2BE872B60FC7CA0851C` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_rtc.h | 21629 | `95C609274EFF580BCFF1223923EC9FCD5AF09AC2AB779FDCE3E412BEC5AAFA24` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_rtc_ex.h | 15521 | `A8A623B5CDE8BCF34001A95A56C556F1412C996491A9155CCC7BE4B734ACDC67` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_sd.h | 38301 | `6E7E39CAA773DD6DCC36D0AB41FF9577A2D354BEBF906D6310E6584094F99F2E` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_smartcard.h | 38113 | `890BDD80DD44F766F7EFAFB68E5878D1A69A2E6E845E9A856134EDE4F1A19A73` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_spi.h | 32314 | `597A9C4033AF4DFAACBBF6053CABF4FE03277008F3B9A4E6113AA5D1C0FCEE5C` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_sram.h | 8879 | `17351A94760EEA771361C479361DAC6E1762487A632310B4EB620155FB30C6C9` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_tim.h | 124307 | `BDA9F5E5222DAA9DDAE23B86B8FD449699DDCCF57681B3D1AFE72598242887DB` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_tim_ex.h | 9839 | `0A923547D8A6B89D4060F7E4A18602CAF393CA86A54E32452FCB8042959AC667` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_uart.h | 46569 | `8E75EA51307D1FCAE4E06CCD1189EC325059CF8E36E18E501BC064B142D7AAA0` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_usart.h | 29783 | `CB6A74E2061CD4E974B3C0374BE63B4AE68AC31908F6EE8DB2809C1B09004303` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_wwdg.h | 10465 | `0C7DFF42AD50588668500A78A1DA24EF043F7A8B368FCC8F61B4F56EF3957C6D` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_adc.h | 232340 | `4412A844880FC5366F67B7D9FB995B9D9EF455C11EDC61835B32915612C996EC` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_bus.h | 46029 | `B360C7B3EAAF5F6E0F31E82DA8C80B8AD8E8BD0140F862B1B39E896C5E4C39F5` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_cortex.h | 24842 | `D82911CC3F992C37833714ADFF86A4F01D9E73F11AC941542452DA8DBDE17A5A` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_crc.h | 5533 | `9B49261CF3C623F2CDC7533A55BE9B710A375F4F6FA9D63058450BE0F2EDFE11` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_dac.h | 64452 | `03DC9490E107D9AB0AA40CAD76F8ED64278A0A2237361E542D0EF49CB4E773D2` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_dma.h | 78189 | `6F5CB4301D1BC7B091C2AE2A54759D0E9CBD7D1C1F51ABE78C29589D6EBB4BDF` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_exti.h | 32092 | `E42125589DD081BAEEBABED85B1169DE8A686296F52D045AD4B94D339EE5FEC7` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_fsmc.h | 40971 | `1376A425DBD6380DE60456CBB1F5E141557BB42B50A431681CDA2F4DCD3B46AE` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_gpio.h | 90375 | `4FD337FAF49F88F0672AE22E2617E7863F5E645F6A26BF32265F0EAD69DDC914` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_i2c.h | 64570 | `C9BADCCF9C259342E7032D3935CA7DC286BBB401A5D37AD8B20F570D6E41E09D` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_iwdg.h | 9400 | `7C18131C24D050E1574EB7214A043C480AEC3B309FE84FA5B241524072238C8C` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_pwr.h | 13019 | `223C5181AA6D33E4A63873CBFED1F4FB88E6DD849DAED35F64F2F7251EB15974` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_rcc.h | 86286 | `B0BC6387C98DAFDDC6268672E6EB87D713AFA60CAEB2C5F4E6AAB07213E2C099` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_rtc.h | 32135 | `EDAF97B29447364AF69CE9CACC211240C919F05C34B233A737363DD1DD5960CF` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_sdmmc.h | 61708 | `2E733C5B345342326279AC46A82FE069E1E17F51B789213E436605281054F260` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_spi.h | 65473 | `CD934F955AF02729EAA057E4B104C57084724C499566A70AB3BD66320F8052EB` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_system.h | 24035 | `19EF0655B999DCA8BA1F6A2ECA0FBA55AE03E3FD9649B40DD55B9A85F435B3C6` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_tim.h | 167946 | `F946F89FB6242BF93EC3FF2153F2D4971F9498457D3B9B4D4D2F7031237B1769` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_usart.h | 101754 | `36E1E830C8939674C6AC3DF4699DB1F1369243D4F2BC3ABC264186A7CCD11FD3` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_usb.h | 26166 | `A0F2FD616F443DB8E9C9EEED2C43F398D6703A4167C22E028AA8C5DAEA701A88` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_utils.h | 8880 | `5F102D18314A8BD2F5C539C85AE6008B0E0B38A1F5AD66D3578F6D9EBF69B189` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_ll_wwdg.h | 10488 | `0230BB4491FA85A36943C5D15DBDE8AED730D28A63AC7774385DC5E4F2B9C6B6` |
| Drivers/STM32F1xx_HAL_Driver/Inc/stm32_assert_template.h | 2040 | `33359AC539295776D5C5574513FAFA94A731FB6FD9431973ED553533E5715600` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c | 20864 | `6650FCFA559DCA126B91C6C0F576E3DEC25DDB26811C55E294CBCC01888A85EC` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c | 50160 | `6225A5FA2D7B7C17549E566FD5F556BEE1DC3FD23EA24EFDBABC2100AA58B143` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c | 30730 | `EB7503A6A1F0B7BFD7488BD851AE3CAC2B88194F344847C74E0A15E563B41DB1` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c | 21479 | `A2E12792C9D49EECDC567AB9FC1A49ED90723007F9579B702C28960DFD83E121` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c | 4149 | `5EC9F22DE1B3EE82E8437B3BA52725F8F031CF9075848582A0A297D4D1176450` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c | 19790 | `024427C57B2F4413FE95806FB5E91B3703093D9B132F4976BD9A1525989F6E23` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c | 28315 | `0662E01264BE0233A4F8F2206DADF1C223CAA17FD54C3B0E2D4A1AE771BD9C2B` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c | 30282 | `7EA731EB5AAEE6092F84738E5A8A2AF5E78065619A49000D4949449855F0F601` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c | 36479 | `E89CE67F60E1A9A99B106A3FC87ED8B4E1DDF30091A74C45158659266A7DC4C0` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c | 21362 | `EBACEA8F41A177A467CF90344705F84F60FE60747ED86524D56E0C3DEA7083F4` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c | 15601 | `2DD9D28FB9417CE038E4EC6CABDAFFE3045367CA104ACFAD02E564101C794673` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c | 140670 | `70300C43E2CA300B8CE01D721801B649DEAF1E50648A34A399071917CCD7215F` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c | 252916 | `0CC71075E2A4DC66D8F7A931BE51C4F6EE582FA97E5435AFE2FBE31BBFF6ABFE` |
| Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c | 81375 | `1C79C5F373348F84059268327C44D570E30B2FC56F3A11E33B6E9743FF39551D` |
| Drivers/CMSIS/Include/cmsis_armcc.h | 28208 | `57B8D88CD03152F60BF74DE0149EC979AE20210D86AB06E837AA69C66F27D615` |
| Drivers/CMSIS/Include/cmsis_armclang.h | 56497 | `DA29DBAD3CF27F6C86437AF6BD0B302E105210742AD7A602BEA51B104F1661C9` |
| Drivers/CMSIS/Include/cmsis_compiler.h | 9014 | `CD3CBD1C771A5252827A14C75A12E7ED975F76B9FC1577D53A74BA0E7C1AF6C3` |
| Drivers/CMSIS/Include/cmsis_gcc.h | 62344 | `8C16D51F7884A9FC07C5CBE6F2BCD24E5E092DDC519F71D36740C2D2E2FF6A27` |
| Drivers/CMSIS/Include/cmsis_iccarm.h | 28414 | `A600765888C69EE60CD2D01D77356902D4AE5FA98AA7E1D73703663C96B7F87A` |
| Drivers/CMSIS/Include/cmsis_version.h | 1716 | `280B9FC8739AEFA878D47E68D7ADBD4908A15F98EC193CDBA1A4B514133F5C34` |
| Drivers/CMSIS/Include/core_armv8mbl.h | 97995 | `220D90F3F99126E656C31C32F099FF766B32CF487F7DD77388E4C9A78E1898C8` |
| Drivers/CMSIS/Include/core_armv8mml.h | 168914 | `825A8799D71F3FA29690B1597DFC2DFB660B103D44E20B8185F26B4F3B4FFDF0` |
| Drivers/CMSIS/Include/core_cm0.h | 42208 | `1BEAA04C859225F75142D055E9C5DACFC9AC1D88C82934177F745B9C5F3F4094` |
| Drivers/CMSIS/Include/core_cm0plus.h | 50464 | `C953256E1C1B1CD6ACCBBFCA5D47C09A9B0534F3147B125E17E59C0AE6C30ED7` |
| Drivers/CMSIS/Include/core_cm1.h | 43456 | `460281028A261740CD5EC86840BED01C480D70BA5B54A0F9938D4CEFADFD486B` |
| Drivers/CMSIS/Include/core_cm23.h | 104628 | `6303EFEC943CA0F9ECCDAB8FC39200D6F95DC943D4051198E0D49F24395028A5` |
| Drivers/CMSIS/Include/core_cm3.h | 111854 | `368296087A925CF78133D27E0854D748605441915E0F4FECE93F2F97EEA68ADB` |
| Drivers/CMSIS/Include/core_cm33.h | 175600 | `7F1B6A76B119E1E1680F756DA897929F63D98133442C5115678AF60ADF20F1A8` |
| Drivers/CMSIS/Include/core_cm4.h | 123607 | `738DFF259C25E68ED2529BAF425315D8DAE1AF5DAC6C11B66BFDEF82E78E86C9` |
| Drivers/CMSIS/Include/core_cm7.h | 148588 | `6B75E3BEF1EB81E0D3E2C311AD26FCDBAC4CDE2CD3462A62B81A23F6AEC0E55F` |
| Drivers/CMSIS/Include/core_sc000.h | 47249 | `D1528D13FBF91DA620EF977E99802D83DED093C0521C04CAD128DA23E3001524` |
| Drivers/CMSIS/Include/core_sc300.h | 110523 | `FA514EA0CF5327B0523C2BDFEBFB76FD2E69F972E5F60FBFB94ADE56B31C722B` |
| Drivers/CMSIS/Include/mpu_armv7.h | 11711 | `B0B6DA4EC744B5F547A94E60E24B0A6FF6BB7307D3F81A83D0B7C3A64DC29326` |
| Drivers/CMSIS/Include/mpu_armv8.h | 10958 | `8280A1D4D3D25F60CDC05B84718CF504F02B7265FC3B7C5790CB29CD7DCF116C` |
| Drivers/CMSIS/Include/tz_context.h | 2757 | `7FCC21FC190F6FD970390E9CAAA6A86D72FD10F9EDF4EFAB36F12AECF1A8BAA0` |
| Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f1xx.h | 10383 | `65E7D272BDC9FFC9E1A6682C20D9F2A1073F3E76B05264F247CBE57987331A07` |
| Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xe.h | 974030 | `DAF857CFF31019C101C0E815A27DF07C7A5B67B01C2A0360661EE5686D1BB713` |
| Drivers/CMSIS/Device/ST/STM32F1xx/Include/system_stm32f1xx.h | 1854 | `8B48FF3463B1C01394BD82E40154B73A288D275E2437923662B40061E88D4A92` |
| Core/Src/system_stm32f1xx.c | 14858 | `15D73C708AD67416BB24E59BA5F110BAA6BDAB217DD910F5DAA33F8E979596D2` |
| startup_stm32f103xe.s | 12492 | `255F63CCD19D16D7326385F2AB1652DEEC3330493CAAC5EC2B1FCE2B57F6C5DB` |
与 `COPIED=111`（108 + 3）一致


## 8. 边界与遗留

- 本文件只记录 HAL/CMSIS 来源与取回证据；不烧录、不改跳线、不改驱动、不升级 J-Link FW、未终止任何 J-Link 进程，未创建 `.vscode/` / `launch.json` / `.ioc` / 阶段 3/4 业务代码。
- 临时检出目录位于 `$env:TEMP\stage2_probe\`（工作区外），仅作取回来源；`Drivers/STM32F1xx_HAL_Driver/` 84 个文件是工程实际编译输入，后续构建以工作区文件为准。
- 链接脚本与启动文件按官方 `STM32F103xE`（512K FLASH / 64K RAM）模板；`@docs/hardware_pins.md` 第 39–41 行把 MCU 内核/Flash/SRAM 容量列为「待人工确认」，本阶段**不把官方模板容量回填为锁定版实测结论**，仅作为工程构建配置，容量须在阶段 5 前人工或调试器核对。
- 版本与哈希以本文件为唯一证据；后续 `generate-stage2-skeleton` 只引用 `@docs/hardware_pins.md` 的锁定引脚，不重复解析原理图、不用手册推断覆盖实测结论。
