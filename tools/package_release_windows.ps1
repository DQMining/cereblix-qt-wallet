# Build and package a portable Windows x64 release zip.
param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$Config = "Release",
    [switch]$SkipBuild,
    [switch]$SkipDeploy
)

$ErrorActionPreference = "Stop"

$QtWallet = Join-Path $RepoRoot "qt-wallet"
$BuildDir = Join-Path $QtWallet "build\$Config"
$OutDir = Join-Path $RepoRoot "dist"
$Stage = Join-Path $OutDir "cereblix-qt-wallet-windows-x64"
$ZipPath = Join-Path $OutDir "cereblix-qt-wallet-windows-x64.zip"

if (-not $SkipBuild) {
    Write-Host "Building $Config ..."
    Push-Location $QtWallet
    cmake --build build --config $Config
    Pop-Location
}

if (-not $SkipDeploy) {
    & (Join-Path $RepoRoot "tools\deploy.ps1") -RepoRoot $RepoRoot -Config $Config
}

if (-not (Test-Path (Join-Path $BuildDir "cereblix-qt-wallet.exe"))) {
    throw "Executable not found after build: $BuildDir\cereblix-qt-wallet.exe"
}

if (Test-Path $Stage) { Remove-Item $Stage -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Stage | Out-Null

Copy-Item (Join-Path $BuildDir "cereblix-qt-wallet.exe") $Stage -Force
Get-ChildItem $BuildDir -Filter "*.dll" | Copy-Item -Destination $Stage -Force
foreach ($dir in @("platforms", "tls", "networkinformation")) {
    $src = Join-Path $BuildDir $dir
    if (Test-Path $src) {
        Copy-Item $src (Join-Path $Stage $dir) -Recurse -Force
    }
}

@"
Cereblix Qt Wallet — Windows x64

Unzip this folder and run:
  cereblix-qt-wallet.exe

Includes Qt TLS plugins for https://cereblix.com/api

Wallet file: %USERPROFILE%\.cereblix\wallet.json (same as the Go CLI wallet)

Self-test:
  cereblix-qt-wallet.exe --self-test
"@ | Set-Content -Path (Join-Path $Stage "README.txt") -Encoding UTF8

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
if (Test-Path $ZipPath) { Remove-Item $ZipPath -Force }
Compress-Archive -Path $Stage -DestinationPath $ZipPath -CompressionLevel Optimal

$sizeMb = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)
Write-Host ""
Write-Host "Created: $ZipPath ($sizeMb MB)"
Write-Host "Folder:  $Stage"
