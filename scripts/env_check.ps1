# Stage 0 environment check - STM32F103ZET6 RedBull board / CodeBuddy IDE workflow.
# Read-only, idempotent, offline. Never writes machine-level or user-level environment variables.
#
# One-command run (from the workspace root, PowerShell):
#   powershell -ExecutionPolicy Bypass -File ".\scripts\env_check.ps1"
#
# Paste the ENTIRE output back to the AI. The AI issues the stage-0 readiness verdict
# following the acceptance gate in docs/env_check.md.
#
# Scope: stage 0 only. No firmware code, no CMakeLists.txt, no .ioc, no debug config.

$ErrorActionPreference = 'SilentlyContinue'

$Pass = 0
$Fail = 0
$Missing = New-Object System.Collections.Generic.List[string]

function Publish {
    param(
        [Parameter(Mandatory = $true)][string]$Level,    # MUST | OPTIONAL | INFO
        [Parameter(Mandatory = $true)][string]$Item,
        [Parameter(Mandatory = $true)][string]$Status,   # PASS | FAIL | FOUND | MISSING
        [string]$Detail = ''
    )
    $line = "$Level $Status $Item"
    if ($Detail -ne '') { $line += ' | ' + $Detail }
    Write-Output $line
    if ($Level -eq 'MUST') {
        if ($Status -eq 'PASS') { $script:Pass++ }
        else { $script:Fail++; $script:Missing.Add($Item) }
    }
}

function Get-VersionLine {
    param([Parameter(Mandatory = $true)][string]$CommandName)
    if (-not (Get-Command $CommandName -ErrorAction SilentlyContinue)) { return $null }
    $line = & $CommandName '--version' 2>&1 | Select-Object -First 1
    if ($null -eq $line) { return $null }
    return ([string]$line).Trim()
}

Write-Output '=== STAGE0 ENV CHECK (read-only, offline, idempotent) ==='
Write-Output ("WORKSPACE=" + (Split-Path -Parent $PSScriptRoot))

# ---------------------------------------------------------------------------
# Portable tools were unpacked by AI during stage 0 and are NOT in the
# persistent PATH. Augment only the CURRENT PROCESS $env:PATH (in memory).
# This is not a system-state write and is fully reversible by closing the shell.
# ---------------------------------------------------------------------------
$KnownBins = @(
    'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\cmake_x64\cmake-4.5.0-rc3-windows-x86_64\bin',
    'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\ninja\bin',
    'C:\Program Files\SEGGER\JLink_V818',
    'C:\Program Files (x86)\Arm GNU Toolchain arm-none-eabi\14.2 rel1\bin'
)
$added = 0
foreach ($bin in $KnownBins) {
    if ((Test-Path -LiteralPath $bin) -and (($env:PATH -split ';') -notcontains $bin)) {
        $env:PATH = $bin + ';' + $env:PATH
        $added++
    }
}
Write-Output "INFO SESSION_PATH_AUGMENTED dirs=$added | current process only, no persistent change"

# ---------------------------------------------------------------------------
# MUST 1-4: the four --version commands required by the workflow acceptance gate
# ---------------------------------------------------------------------------
foreach ($tool in @('cmake', 'ninja', 'arm-none-eabi-gcc', 'arm-none-eabi-gdb')) {
    $v = Get-VersionLine $tool
    if ($v) {
        $src = (Get-Command $tool -ErrorAction SilentlyContinue).Source
        Publish -Level MUST -Item $tool -Status PASS -Detail "version=$v path=$src"
    }
    else {
        Publish -Level MUST -Item $tool -Status FAIL -Detail 'Get-Command failed or --version returned nothing'
    }
}

$sizeVer = Get-VersionLine 'arm-none-eabi-size'
if ($sizeVer) {
    Publish -Level OPTIONAL -Item 'arm-none-eabi-size' -Status PASS -Detail "version=$sizeVer"
}
else {
    Publish -Level OPTIONAL -Item 'arm-none-eabi-size' -Status MISSING -Detail 'stage 2 uses it for Flash/RAM report'
}

# ---------------------------------------------------------------------------
# MUST 5: J-Link GDB Server. Do NOT execute it here: --help probing belongs to
# stage 5 (workflow: never invent J-Link CLI arguments).
# ---------------------------------------------------------------------------
$jlinkPath = $null
$jlinkCmd = Get-Command JLinkGDBServerCL -ErrorAction SilentlyContinue
if ($jlinkCmd) { $jlinkPath = $jlinkCmd.Source }

if (-not $jlinkPath) {
    foreach ($c in @(
        'C:\Program Files\SEGGER\JLink_V818\JLinkGDBServerCL.exe',
        'C:\Program Files\SEGGER\JLink\JLinkGDBServerCL.exe',
        'C:\Program Files (x86)\SEGGER\JLink\JLinkGDBServerCL.exe'
    )) {
        if (Test-Path -LiteralPath $c) { $jlinkPath = $c; break }
    }
}
if (-not $jlinkPath) {
    foreach ($r in @('C:\Program Files\SEGGER', 'C:\Program Files (x86)\SEGGER')) {
        if (Test-Path -LiteralPath $r) {
            $hit = Get-ChildItem -LiteralPath $r -Recurse -Filter 'JLinkGDBServerCL.exe' -File -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($hit) { $jlinkPath = $hit.FullName; break }
        }
    }
}
if ($jlinkPath) {
    $bytes = (Get-Item -LiteralPath $jlinkPath -ErrorAction SilentlyContinue).Length
    Publish -Level MUST -Item 'JLinkGDBServerCL' -Status PASS -Detail "path=$jlinkPath bytes=$bytes | file probed only, NOT executed"
    Write-Output "INFO JLINK_GDB_SERVER_ABS=$jlinkPath"
}
else {
    Publish -Level MUST -Item 'JLinkGDBServerCL' -Status FAIL -Detail 'JLinkGDBServerCL.exe not found; record absolute path, never rely on PATH'
}

# ---------------------------------------------------------------------------
# MUST 6: STM32F1 SVD. Real file name observed on this machine is
# STM32F103xx.svd (not the STM32F103.svd placeholder in the workflow doc).
# ---------------------------------------------------------------------------
$svdRoots = @(
    'C:\Users\HenrryBian\AppData\Local\Programs\EmbeddedTools\Keil.STM32F1xx_DFP.2.4.1\SVD',
    'C:\Users\HenrryBian\AppData\Roaming\Keil',
    'C:\Users\HenrryBian\AppData\Local\Arm',
    'C:\Program Files\STMicroelectronics',
    'C:\Program Files (x86)\STMicroelectronics',
    'C:\Program Files (x86)\Keil',
    'C:\Keil_v5'
)
$svd = $null
foreach ($r in $svdRoots) {
    if (Test-Path -LiteralPath $r) {
        $hit = Get-ChildItem -LiteralPath $r -Recurse -Filter 'STM32F103*.svd' -File -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($hit) { $svd = $hit.FullName; break }
    }
}
if ($svd) {
    $bytes = (Get-Item -LiteralPath $svd -ErrorAction SilentlyContinue).Length
    Publish -Level MUST -Item 'STM32F1_SVD' -Status PASS -Detail "path=$svd bytes=$bytes"
    Write-Output "INFO SVD_ABS=$svd"
}
else {
    Publish -Level MUST -Item 'STM32F1_SVD' -Status FAIL -Detail 'STM32F103*.svd not found; search CubeMX / Keil Pack install dirs'
}

# ---------------------------------------------------------------------------
# MUST 7-8: CodeBuddy IDE front end and Cortex-Debug extension (primary debug path)
# ---------------------------------------------------------------------------
$cbProc = Get-Process -Name 'CodeBuddy CN' -ErrorAction SilentlyContinue | Select-Object -First 1
$cbExe = $null
if ($cbProc -and $cbProc.Path) { $cbExe = $cbProc.Path }
elseif (Test-Path -LiteralPath 'D:\Program Files\CodeBuddy CN\CodeBuddy CN.exe') {
    $cbExe = 'D:\Program Files\CodeBuddy CN\CodeBuddy CN.exe'
}
if ($cbExe) {
    Publish -Level MUST -Item 'CodeBuddy_IDE' -Status PASS -Detail "path=$cbExe"
}
else {
    Publish -Level MUST -Item 'CodeBuddy_IDE' -Status FAIL -Detail 'CodeBuddy CN install / process not found'
}

$extRoot = Join-Path $env:USERPROFILE '.codebuddycn\extensions'
$cdDir = Get-ChildItem -LiteralPath $extRoot -Directory -Filter 'marus25.cortex-debug*' -ErrorAction SilentlyContinue | Select-Object -First 1
if ($cdDir) {
    $ver = 'unknown'
    $pj = Join-Path $cdDir.FullName 'package.json'
    if (Test-Path -LiteralPath $pj) {
        $meta = Get-Content -LiteralPath $pj -Raw -ErrorAction SilentlyContinue | ConvertFrom-Json
        if ($meta.version) { $ver = $meta.version }
    }
    $deps = @(
        'mcu-debug.debug-tracker-vscode',
        'mcu-debug.memory-view',
        'mcu-debug.rtos-views',
        'mcu-debug.peripheral-viewer'
    )
    $depCount = 0
    foreach ($d in $deps) {
        if (Get-ChildItem -LiteralPath $extRoot -Directory -Filter "$d*" -ErrorAction SilentlyContinue | Select-Object -First 1) { $depCount++ }
    }
    Publish -Level MUST -Item 'Cortex-Debug_ext' -Status PASS -Detail "dir=$($cdDir.Name) version=$ver deps=$depCount/4 | primary path available for stage 5"
}
else {
    Publish -Level MUST -Item 'Cortex-Debug_ext' -Status FAIL -Detail 'extension not found; stage 5 falls back to command-line GDB (does not block stage 0)'
}

# ---------------------------------------------------------------------------
# MUST 9: schematic PDF exists and is readable. Stage 0 must NOT parse it.
# Wildcard avoids hard-coding the Chinese file name (PowerShell 5.1 encoding).
# ---------------------------------------------------------------------------
$root = Split-Path -Parent $PSScriptRoot
$pdf = Get-ChildItem -LiteralPath $root -Filter '*.pdf' -File -ErrorAction SilentlyContinue | Select-Object -First 1
$readable = $false
if ($pdf) {
    try {
        $fs = [System.IO.File]::OpenRead($pdf.FullName)
        $fs.Close()
        $readable = $true
    }
    catch { $readable = $false }
}
if ($pdf -and $readable -and $pdf.Length -gt 0) {
    Publish -Level MUST -Item 'Schematic_PDF' -Status PASS -Detail "bytes=$($pdf.Length) openread=True | NOT parsed in stage 0"
}
else {
    Publish -Level MUST -Item 'Schematic_PDF' -Status FAIL -Detail 'no readable *.pdf in workspace root; check file name 红牛开发板电路图(RedBull Schematic).pdf'
}

# ---------------------------------------------------------------------------
# OPTIONAL items and informational facts
# ---------------------------------------------------------------------------
$mx = Get-ChildItem -LiteralPath 'C:\Program Files\STMicroelectronics' -Directory -Filter 'STM32Cube*' -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $mx) {
    $mx = Get-ChildItem -LiteralPath 'C:\Program Files (x86)\STMicroelectronics' -Directory -Filter 'STM32Cube*' -ErrorAction SilentlyContinue | Select-Object -First 1
}
if ($mx) {
    Publish -Level OPTIONAL -Item 'STM32CubeMX' -Status PASS -Detail "path=$($mx.FullName)"
}
else {
    Publish -Level OPTIONAL -Item 'STM32CubeMX' -Status MISSING -Detail 'no winget package; .ioc is optional in stage 2; install from ST official package if needed'
}

$wgCmd = Get-Command winget -ErrorAction SilentlyContinue
if ($wgCmd) {
    Publish -Level OPTIONAL -Item 'winget' -Status PASS -Detail "path=$($wgCmd.Source) version=$((& $wgCmd.Source '--version' 2>&1 | Select-Object -First 1))"
}
else {
    Publish -Level OPTIONAL -Item 'winget' -Status MISSING -Detail 'package manager unavailable'
}

$codeCmd = Get-Command code -ErrorAction SilentlyContinue
if ($codeCmd) {
    Publish -Level OPTIONAL -Item 'VSCode_CLI' -Status PASS -Detail "path=$($codeCmd.Source) | reference only, dev front end is CodeBuddy IDE"
}

Write-Output 'INFO CODEBUDDY_UI | Plan/Craft/Ask, built-in terminal, @ context: session-verified; see docs/env_check.md'
Write-Output 'INFO HARDWARE | oscilloscope/logic analyzer optional, not probed by script; J-Link + variable/log observation is the substitute'
Write-Output 'INFO SWD_SAFETY | stage 0 installs software only: no board connected, no power, no flashing, no jumper change, no option bytes, no read protection'
Write-Output 'INFO NO_FIRMWARE | stage 0 generated no .c/.h/.s/.ld, no CMakeLists.txt, no .ioc, no debug config'

# ---------------------------------------------------------------------------
# Summary. The AI - not this script - issues the final verdict.
# ---------------------------------------------------------------------------
Write-Output '=== SUMMARY ==='
Write-Output "MUST_PASS=$Pass MUST_FAIL=$Fail"
if ($Fail -gt 0) {
    Write-Output ("MISSING_ITEMS=" + ($Missing -join ','))
    Write-Output 'VERDICT_HINT=NOT_READY'
}
else {
    Write-Output 'MISSING_ITEMS=none'
    Write-Output 'VERDICT_HINT=ALL_MUST_PASS'
}
Write-Output 'NOTE=AI issues the final stage-0 verdict and failure fallback per docs/env_check.md acceptance gate'
Write-Output '=== END ==='
