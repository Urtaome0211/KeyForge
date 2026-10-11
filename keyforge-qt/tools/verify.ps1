# ============================================================
# verify.ps1 — 键锋 KEYFORGE Qt 版集成验证流水线
# 流程：配置 cmake（重扫 QML）→ 构建 → --selftest → --smoke
#       可选 -Package 追加：windeployqt 打包 → 打包产物 --selftest
# 用法：pwsh -File tools\verify.ps1 [-Package]
# 注意：GUI 程序 stdout 直接捕获为空，统一用 Start-Process
#       重定向到 %TEMP% 文件 + WaitForExit 超时读取。
# ============================================================
param([switch]$Package)
$ErrorActionPreference = "Stop"

$root    = Split-Path -Parent $PSScriptRoot          # keyforge-qt\ 根目录
$env:PATH = "D:\Qt\Tools\llvm-mingw2217_64\bin;D:\Qt\6.12.0\llvm-mingw_64\bin;D:\Qt\Tools\CMake_64\bin;D:\Qt\Tools\Ninja;$env:PATH"
Remove-Item Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue

# ---- 1. 配置（QML 文件可能刚落地，必须重扫；CONFIGURE_DEPENDS 会自动触发） ----
Write-Host "== 1/5 cmake 配置 =="
cmake -S $root -B (Join-Path $root "build") -G Ninja -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_PREFIX_PATH=D:/Qt/6.12.0/llvm-mingw_64
if ($LASTEXITCODE -ne 0) { throw "cmake 配置失败" }

# ---- 2. 构建 ----
Write-Host "== 2/5 cmake 构建 =="
cmake --build (Join-Path $root "build")
if ($LASTEXITCODE -ne 0) { throw "cmake 构建失败" }

$exe = Join-Path $root "build\Keyforge.exe"
if (-not (Test-Path $exe)) { throw "构建产物不存在: $exe" }

# ---- 3. 自测（QCoreApplication 模式，不依赖 QPA 插件） ----
Write-Host "== 3/5 --selftest =="
$out = Join-Path $env:TEMP "kf_selftest_out.txt"
$err = Join-Path $env:TEMP "kf_selftest_err.txt"
Remove-Item $out, $err -ErrorAction SilentlyContinue
$p = Start-Process $exe -ArgumentList "--selftest" -PassThru -RedirectStandardOutput $out -RedirectStandardError $err -NoNewWindow
if (-not $p.WaitForExit(30000)) { $p.Kill(); throw "--selftest 超时" }
# 注意：PS 5.1 在 -NoNewWindow + 重定向组合下 Process.ExitCode 恒为空
# （Refresh/Get-Process 均无效），故以输出标记行 + 超时作为成功判据。
$txt = (Get-Content $out -Raw -ErrorAction SilentlyContinue)
if ($txt -notmatch "SELFTEST OK") {
    $errTxt = (Get-Content $err -Raw -ErrorAction SilentlyContinue)
    throw "--selftest 失败（未输出 SELFTEST OK）。stdout: $out / stderr: $err`n$errTxt"
}
Write-Host "  SELFTEST OK"

# ---- 4. 无头冒烟（offscreen 平台加载 QML） ----
Write-Host "== 4/5 --smoke =="
$out2 = Join-Path $env:TEMP "kf_smoke_out.txt"
$err2 = Join-Path $env:TEMP "kf_smoke_err.txt"
Remove-Item $out2, $err2 -ErrorAction SilentlyContinue
$env:QT_QPA_PLATFORM = "offscreen"
$p = Start-Process $exe -ArgumentList "--smoke" -PassThru -RedirectStandardOutput $out2 -RedirectStandardError $err2 -NoNewWindow
if (-not $p.WaitForExit(30000)) { $p.Kill(); throw "--smoke 超时" }
# 退出码判据同上（PS 5.1 ExitCode 恒空），以 SMOKE OK 标记行判断。
$smokeTxt = (Get-Content $out2 -Raw -ErrorAction SilentlyContinue)
if ($smokeTxt -notmatch "SMOKE OK") {
    $errTxt2 = (Get-Content $err2 -Raw -ErrorAction SilentlyContinue)
    throw "--smoke 失败（未输出 SMOKE OK）。stderr:`n$errTxt2"
}
Write-Host "  SMOKE OK"
$warn = (Get-Content $err2 -Raw -ErrorAction SilentlyContinue)
if ($warn -and $warn.Trim()) {
    Write-Host "  [注意] smoke stderr 有输出（可能是 QML 警告，需人工确认无碍）:"
    Write-Host $warn
} else {
    Write-Host "  （QML 无警告输出）"
}
Remove-Item Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue

# ---- 5. 打包 ----
if ($Package) {
    Write-Host "== 5/5 windeployqt 打包 =="
    & (Join-Path $PSScriptRoot "package.ps1")
    if ($LASTEXITCODE -ne 0) { throw "package.ps1 失败" }
    $distExe = Join-Path $root "dist\Keyforge\Keyforge.exe"
    if (-not (Test-Path $distExe)) { throw "打包产物不存在: $distExe" }
    # 注意：dist 里没有 offscreen 插件（windeployqt 只部署目标平台
    # qwindows），不能对 dist 跑 --smoke；改用 --selftest 在干净环境
    # （无 Qt PATH）下验证打包产物的 DLL 自包含性。窗口级验证：
    # 直接双击 dist\Keyforge\Keyforge.exe 确认界面正常。
    $env:PATH = "C:\Windows\System32;C:\Windows"
    $out3 = Join-Path $env:TEMP "kf_dist_selftest_out.txt"
    $err3 = Join-Path $env:TEMP "kf_dist_selftest_err.txt"
    Remove-Item $out3, $err3 -ErrorAction SilentlyContinue
    $p = Start-Process $distExe -ArgumentList "--selftest" -PassThru -RedirectStandardOutput $out3 -RedirectStandardError $err3 -NoNewWindow
    if (-not $p.WaitForExit(30000)) { $p.Kill(); throw "打包产物 --selftest 超时" }
    # 退出码判据同上（PS 5.1 ExitCode 恒空），以 SELFTEST OK 标记行判断。
    $txt3 = (Get-Content $out3 -Raw -ErrorAction SilentlyContinue)
    if ($txt3 -notmatch "SELFTEST OK") {
        $errTxt3 = (Get-Content $err3 -Raw -ErrorAction SilentlyContinue)
        throw "打包产物 --selftest 失败（未输出 SELFTEST OK）。stderr:`n$errTxt3"
    }
    Write-Host "  DIST SELFTEST OK -> $distExe"
} else {
    Write-Host "== 5/5 跳过打包（如需打包加 -Package） =="
}

Write-Host "== 全部通过 =="
