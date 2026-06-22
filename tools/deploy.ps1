param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$BuildDir = Join-Path $RepoRoot "qt-wallet\build\$Config"
$QtBin = Join-Path $RepoRoot "qt6\6.8.3\msvc2022_64\bin"
$QtPlugins = Join-Path (Split-Path $QtBin -Parent) "plugins"
$VcpkgDll = Join-Path $RepoRoot "qt-wallet\build\vcpkg_installed\x64-windows\bin\libsodium.dll"

if (-not (Test-Path $BuildDir)) {
    throw "Build output not found: $BuildDir (run cmake --build first)"
}
if (-not (Test-Path $QtBin)) {
    throw "Qt bin not found: $QtBin"
}
if (-not (Test-Path $VcpkgDll)) {
    throw "libsodium.dll not found: $VcpkgDll"
}

New-Item -ItemType Directory -Force -Path "$BuildDir\platforms" | Out-Null
New-Item -ItemType Directory -Force -Path "$BuildDir\tls" | Out-Null
New-Item -ItemType Directory -Force -Path "$BuildDir\networkinformation" | Out-Null

Copy-Item "$QtBin\Qt6Core.dll","$QtBin\Qt6Gui.dll","$QtBin\Qt6Widgets.dll","$QtBin\Qt6Network.dll" $BuildDir -Force
Copy-Item $VcpkgDll $BuildDir -Force
Copy-Item "$QtPlugins\platforms\qwindows.dll" "$BuildDir\platforms\" -Force
Copy-Item "$QtPlugins\tls\qschannelbackend.dll" "$BuildDir\tls\" -Force
Copy-Item "$QtPlugins\networkinformation\qnetworklistmanager.dll" "$BuildDir\networkinformation\" -Force

$Cereblixd = Join-Path $RepoRoot "cereblix\cereblixd.exe"
if (Test-Path $Cereblixd) {
    Copy-Item $Cereblixd $BuildDir -Force
    Write-Host "Copied cereblixd.exe"
}

Write-Host "Deployed runtime DLLs to $BuildDir"
