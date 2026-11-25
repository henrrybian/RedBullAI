# =====================================================================
# 阶段 5：一键烧录脚本（PowerShell）
# 生成日期：2026-11-12 ｜ 依据：docs/env_check.md 8.3 实测绝对路径 +
#          V8.18 J-Link Commander 命令列表实测（详见 scripts/flash.jlink 头注）
#
# ⚠️⚠️⚠️ 执行前需用户确认 ⚠️⚠️⚠️
#   本脚本会擦写目标芯片 Flash（J-Link LoadFile + VerifyBin）。按照本项目
#   烧录授权边界（阶段 3/4 先例：每次烧录均需用户当轮显式授权），运行本脚本前
#   必须获得用户明确确认（例如回复「确认烧录」）。脚本运行时会再次提示确认。
#
# 用法（工作区根目录、CodeBuddy 内置终端）：
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\flash.ps1
#   powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\flash.ps1 -ConfirmFlash yes   # 免交互确认（仍需用户事先授权）
#
# 前置（脚本自动只读检查，任一失败即中止）：
#   1) build/Debug/RedBull_Buzzer_PWM.hex/.bin 存在；
#   2) J-Link USB 在线（PnP 枚举，只读）；
#   3) 端口 2331 空闲（避免与其他 GDB Server 冲突）；
#   4) 无正在运行的 JLink/JFlash 进程占用调试器（只检查，不终止任何进程）。
#
# 烧录后动作（flash.jlink 内置）：LoadFile(.hex) → VerifyBin(.bin @0x08000000)
#   → Reset → Go（复位运行，从主 Flash 启动，前提 BOOT0=0，见 docs/debug_guide.md）。
# =====================================================================

param(
    [string]$ConfirmFlash = ''
)

$ErrorActionPreference = 'Continue'

# ---- 实测绝对路径（docs/env_check.md 8.3，不依赖 PATH） ----
$JLinkExe      = 'C:\Program Files\SEGGER\JLink_V818\JLink.exe'
$CmdFile       = Join-Path $PSScriptRoot 'flash.jlink'
$WorkspaceRoot = Split-Path -Parent $PSScriptRoot
$HexFile       = Join-Path $WorkspaceRoot 'build\Debug\RedBull_Buzzer_PWM.hex'
$BinFile       = Join-Path $WorkspaceRoot 'build\Debug\RedBull_Buzzer_PWM.bin'

"=== STAGE5 FLASH SCRIPT (J-Link Commander one-click) ==="

# ---- 0) 用户确认门（烧录授权边界） ----
if ($ConfirmFlash -ne 'yes') {
    "CONFIRM_REQUIRED=烧录将擦写目标 Flash（J-Link LoadFile/VerifyBin）。"
    "按本项目烧录授权边界，需用户在会话中明确回复「确认烧录」后才执行。"
    $ans = Read-Host "输入 yes 继续烧录（其他任意输入=中止）"
    if ($ans -ne 'yes') { "FLASH_ABORTED=NOT_AUTHORIZED"; exit 2 }
} else {
    "CONFIRM_FLASH=yes（命令行参数确认；前提=用户已在会话中显式授权）"
}

# ---- 1) 前置只读检查 ----
if (-not (Test-Path -LiteralPath $JLinkExe)) { "FAIL JLinkExe missing: $JLinkExe"; exit 3 }
if (-not (Test-Path -LiteralPath $CmdFile))  { "FAIL cmd file missing: $CmdFile"; exit 3 }
if (-not (Test-Path -LiteralPath $HexFile))  { "FAIL hex missing: $HexFile （先执行 cmake --build build/Debug）"; exit 3 }
if (-not (Test-Path -LiteralPath $BinFile))  { "FAIL bin missing: $BinFile"; exit 3 }
"PASS artifacts: hex=$((Get-Item -LiteralPath $HexFile).Length) bytes, bin=$((Get-Item -LiteralPath $BinFile).Length) bytes"

$jlink = Get-PnpDevice -ErrorAction SilentlyContinue | Where-Object { $_.InstanceId -match 'VID_1366' -and $_.Present }
if (-not $jlink) { "FAIL J-Link not present (PnP). 检查 USB 连接"; exit 4 }
"PASS J-Link present: $($jlink.FriendlyName) [$($jlink.Status)]"

$conn = Get-NetTCPConnection -LocalPort 2331 -ErrorAction SilentlyContinue
if ($conn) { "FAIL port 2331 in use（已有一个 GDB Server 在占用，先由其所有者关闭）"; exit 5 }
"PASS port 2331 free"

$procs = Get-Process -Name 'JLink', 'JLinkGDBServerCL', 'JLinkGDBServer', 'JFlash' -ErrorAction SilentlyContinue
if ($procs) { "FAIL J-Link related processes running（不代为终止，请用户自行关闭后再试）: $($procs.Id -join ',')"; exit 6 }
"PASS no J-Link processes"

# ---- 2) 执行烧录（J-Link Commander + flash.jlink） ----
$outLog = Join-Path $env:TEMP 'stage5_flash_out.log'
$errLog = Join-Path $env:TEMP 'stage5_flash_err.log'
"FLASH_START=$(Get-Date -Format 'HH:mm:ss')"
$p = Start-Process -FilePath $JLinkExe -ArgumentList '-ExitOnError','1','-CommandFile',('"{0}"' -f $CmdFile) `
        -RedirectStandardOutput $outLog -RedirectStandardError $errLog -NoNewWindow -PassThru
if ($p.WaitForExit(120000)) {
    $p.WaitForExit() | Out-Null
    "CMDEXIT=$($p.ExitCode)"
    $exitCode = $p.ExitCode
} else {
    Stop-Process -Id $p.Id -Force
    "FAIL timeout 120s (script's own process)"
    exit 7
}

$outText = Get-Content -LiteralPath $outLog -ErrorAction SilentlyContinue | Out-String
$errText = Get-Content -LiteralPath $errLog -ErrorAction SilentlyContinue | Out-String
"--- J-LINK STDOUT ---"
$outText.TrimEnd()

# ---- 3) 结果判定（扫描 Commander 输出文本；标记均来自实测输出文本） ----
# 成功标记：'Downloading file ...'（LoadFile 开始）/'O.K.'（LoadFile 完成回显）/
#           'Verify successful'（VerifyBin 回显，V8.18 实测原样文本）
# 失败标记：'Could not connect'（连接失败回显）/'Unknown command'（命令不被识别）/退出码非 0
$failed = ($outText + $errText) -match 'Could not connect|Unknown command|Failed to'
$loaded = $outText -match 'Downloading file'
$verOK  = $outText -match 'Verify successful'
if (-not $exitCode) { $exitCode = 0 }
if ($failed -or ($exitCode -ne 0)) {
    "FLASH_RESULT=FAIL（详见上方输出；排查见 docs/debug_guide.md 决策树）"
    exit 8
}
if ($loaded -and $verOK) {
    "FLASH_RESULT=OK（LoadFile 下载 + VerifyBin 校验 + Reset + Go 完成）"
    "NEXT=复位后从主 Flash 启动（BOOT0=0）；打开 COM3 115200-8N1 终端应看到 STAGE3 启动日志与 BUZZER 段切换 INFO。"
    exit 0
}
"FLASH_RESULT=UNKNOWN（未匹配到明确的成功/失败标记，请人工核对上方 J-Link 输出文本）"
exit 9
