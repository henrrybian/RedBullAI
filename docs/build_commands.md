# 阶段 5：CMake 构建命令清单（PowerShell）

> 项目：STM32F103ZET6 红牛开发板蜂鸣器 PWM ｜ 生成日期：2026-11-12 ｜ CodeBuddy IDE + 内置终端
> 依据：`STM32_Buzzer_PWM_Workflow.md` 阶段 5 共用项 1（566 行）、`docs/env_check.md` 8.3 实测绝对路径。
> 原则：工具用**实测绝对路径**调用，不依赖 PATH 硬编码；含空格/括号路径一律加引号；`cmake`/`ninja` 为可移植包，不在持久 PATH，需会话级补齐。
> 本清单全部命令已在 2026-11-12 实际执行验证（证据见第 5 节）。

---

## 1. 会话级 PATH 补齐（每次新开终端先执行一次，不改机器级/用户级环境变量）

```powershell
$env:PATH = 'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\cmake_x64\cmake-4.5.0-rc3-windows-x86_64\bin;' +
            'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\ninja\bin;' + $env:PATH
```

> 来源：`docs/env_check.md` 8.3（cmake 4.5.0-rc3 / ninja 1.13.2 为用户目录可移植包）；第 9 节持久化方案为可选，本阶段不执行。

## 2. 配置 / 构建 / 清理 / 重建

工作区根目录执行（`Set-Location` 到 `c:/Users/HenrryBian/CodeBuddy/20261110233826`）：

```powershell
# 2.1 首次配置（生成 build/Debug）
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 2.2 增量构建
cmake --build build/Debug

# 2.3 清理构建产物（保留 CMake 缓存，重新编译全部目标）
cmake --build build/Debug --clean-first

# 2.4 彻底清理（删除 build/Debug，含 CMake 缓存；下次需重新执行 2.1）
Remove-Item -LiteralPath 'build\Debug' -Recurse -Force -ErrorAction SilentlyContinue

# 2.5 重新配置（改了 CMakeLists.txt / 工具链文件后）
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

> 路径含空格时必须加引号（如工具链文件路径 `cmake/gcc-arm-none-eabi.cmake` 无空格，但 Arm 工具链安装目录含空格，凡手工调用其下 exe 时必须引号包裹）。

## 3. 产物与 size 检查

```powershell
# 3.1 产物清点（.elf/.hex/.bin/.map 四件齐全）
Get-ChildItem -LiteralPath 'build/Debug' -Filter '*.elf' | Select-Object Name, Length
Get-ChildItem -LiteralPath 'build/Debug' -Filter '*.hex' | Select-Object Name, Length
Get-ChildItem -LiteralPath 'build/Debug' -Filter '*.bin' | Select-Object Name, Length
Get-ChildItem -LiteralPath 'build/Debug' -Filter '*.map' | Select-Object Name, Length

# 3.2 size 检查（PowerShell 下原生程序不展开 *.elf 通配符，必须先用 Get-ChildItem 取路径）
$elf = (Get-ChildItem -LiteralPath 'build/Debug' -Filter '*.elf').FullName
& 'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin\arm-none-eabi-size.exe' --format=berkeley $elf
```

### size 输出说明（berkeley 格式五列）

```text
   text     data      bss      dec      hex  filename
  15764      116     2100    17980    0x463c  RedBull_Buzzer_PWM.elf
```

| 列 | 含义 | 归属 |
| --- | --- | --- |
| `text` | 代码 + 常量（Flash） | Flash |
| `data` | 已初始化数据初值（存 Flash、运行时复制到 RAM） | **Flash + RAM 都计** |
| `bss` | 零初始化数据（RAM） | RAM |
| `dec`/`hex` | `text+data+bss` 十进制/十六进制合计 | — |

- **Flash 占用 = `text + data` = 15764 + 116 = 15,880 B**（512 KB 的 3.0%）→ 在 `{MCU}` 512K 内 ✅
- **RAM 占用 = `data + bss` = 116 + 2100 = 2,216 B**（64 KB 的 3.4%）→ 在 64K 内 ✅
- 容量口径来自官方 `STM32F103xE` 链接脚本 `STM32F103ZETx_FLASH.ld`（512K FLASH / 64K RAM）。
- 退出码判据：`$LASTEXITCODE` 应为 `0`；链接阶段允许出现的**非 error 警告**（阶段 2 起已登记）：`_close/_fstat/_isatty/_lseek/_read is not implemented`（nosys 桩，`_write` 已强定义无此警告）与 `LOAD segment with RWX permissions`（官方 ld 模板段属性）。

## 4. 构建配置要点（不得改动）

- Debug 固定 `-O0 -g3`（`CMakeLists.txt` 22–23 行强制 CACHE），编译定义 `-DDEBUG_LOG_LEVEL=4 -DSTM32F103xE -DUSE_HAL_DRIVER`。
- 工具链文件 `cmake/gcc-arm-none-eabi.cmake` 用 `docs/env_check.md` 8.3 实测绝对路径，不依赖 PATH。
- 禁止改用 `-Os`/`-O2` 构建 Debug（单步调试变量会 optimized out，见 `docs/debug_guide.md`）。

## 5. 2026-11-12 实测证据（clean-first 完整重建）

```text
cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug  → CONFIGURE_EXIT=0（Configuring/Generating done）
cmake --build build/Debug --clean-first                      → BUILD_EXIT=0（0 error，仅既有非 error 警告）
产物：RedBull_Buzzer_PWM.elf 905,016 B / .hex 44,759 B / .bin 15,880 B / .map 504,877 B（四件齐全）
size：text 15764 / data 116 / bss 2100 → Flash 15,880 B（512K 的 3.0%）/ RAM 2,216 B（64K 的 3.4%）  SIZE_EXIT=0
```
