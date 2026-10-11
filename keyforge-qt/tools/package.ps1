# ============================================================
# package.ps1 — 键锋 KEYFORGE Qt 版打包脚本
# 作用：把 build\Keyforge.exe 用 windeployqt 打包成可双击运行的独立目录 dist\Keyforge\
# 用法：pwsh -File tools\package.ps1（需先 cmake --build build 成功）
#
# 注意：本机 Qt 6.12.0 llvm-mingw kit 的 DLL 会被 windeployqt 误判为 debug
# 版本，加 --release 会把全部 DLL（含 qwindows 平台插件）过滤掉并报
# "Unable to find the platform plugin."——所以这里不能带 --release。
# ============================================================
$ErrorActionPreference = "Stop"

$root    = Split-Path -Parent $PSScriptRoot          # keyforge-qt\ 根目录
$qtBin   = "D:\Qt\6.12.0\llvm-mingw_64\bin"          # Qt kit（windeployqt 在此）
$ccBin   = "D:\Qt\Tools\llvm-mingw2217_64\bin"       # 编译器运行时（--compiler-runtime 用）
$exe     = Join-Path $root "build\Keyforge.exe"
$dist    = Join-Path $root "dist\Keyforge"
$qmlDir  = Join-Path $root "qml"

if (-not (Test-Path $exe)) { throw "未找到 $exe —— 请先 cmake --build build" }
if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }
New-Item -ItemType Directory -Path $dist | Out-Null
Copy-Item $exe $dist

# 编译器运行时 DLL 需在 PATH 上才能被 windeployqt 找到
$env:PATH = "$ccBin;$qtBin;$env:PATH"

# windeployqt 必须在其自身目录下运行（它启动时需要同目录的 Qt DLL）。
# PS 5.1 会把原生 stderr 当错误记录，配合 $ErrorActionPreference="Stop"
# 会中断脚本——故调用期间临时改为 Continue，靠退出码判断成败。
Push-Location $qtBin
try {
    $prevEAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    & ".\windeployqt.exe" --no-translations --no-opengl-sw `
        --compiler-runtime --qmldir $qmlDir (Join-Path $dist "Keyforge.exe") 2>&1 | ForEach-Object { $_.ToString() }
    $ErrorActionPreference = $prevEAP
    if ($LASTEXITCODE -ne 0) { throw "windeployqt 失败，退出码 $LASTEXITCODE" }
} finally {
    Pop-Location
}

# ============================================================
# 自包含修复（两件必做的事，否则 dist 版启动即退 EXIT=1）：
# ① exe 内嵌的 Keyforge QML 模块（:/Keyforge/）在 Qt 6.12 下
#    运行时解析不到 Main 类型（报 "Module Keyforge contains no
#    type named Main"），构建目录里靠 build\Keyforge\ 这个文件
#    系统模块目录兜底。windeployqt 不会复制应用自己的模块，
#    所以要把 build\Keyforge\{qmldir,Main.qml,Keyforge.qmltypes}
#    手工复制到 dist\Keyforge\Keyforge\，让引擎按 exe 目录导入。
# ② 写 qt.conf（[Paths] Prefix=.）让插件/QML 库路径全部相对
#    dist 目录解析，脱离编译期内嵌的 D:\Qt\... 前缀，实现真正
#    可移植（拷到没装 Qt 的机器也能双击运行）。
# ============================================================
$modSrc = Join-Path $root "build\Keyforge"
$modDst = Join-Path $dist "Keyforge"
New-Item -ItemType Directory -Path $modDst -Force | Out-Null
Copy-Item (Join-Path $modSrc "qmldir")           $modDst -Force
Copy-Item (Join-Path $modSrc "Main.qml")         $modDst -Force
Copy-Item (Join-Path $modSrc "Keyforge.qmltypes") $modDst -Force
Set-Content -Path (Join-Path $dist "qt.conf") -Value "`n[Paths]`nPrefix=.`n" -Encoding ASCII

Write-Host "打包完成: $dist"
Write-Host "验证方式: & '$dist\Keyforge.exe' --smoke   （应输出 SMOKE OK）"
