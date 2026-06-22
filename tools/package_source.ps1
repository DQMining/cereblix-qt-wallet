# Create a source zip safe to share (no wallet, no build artifacts, no Qt install).
param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$OutDir = (Join-Path (Resolve-Path (Join-Path $PSScriptRoot "..")).Path "dist")
)

$ErrorActionPreference = "Stop"

$excludeDirNames = @(
    "build", "qt6", "cereblix", "dist", ".git", ".vs",
    "vcpkg_installed", "CMakeFiles", "cereblix-qt-wallet_autogen"
)
$excludeFilePatterns = @(
    "*.exe", "*.dll", "*.pdb", "*.obj", "*.lib", "*.exp", "*.ilk",
    "wallet.json", "*.wallet"
)

function Should-ExcludePath([string]$FullPath, [string]$Root) {
    $rel = $FullPath.Substring($Root.Length).TrimStart('\', '/')
    if ([string]::IsNullOrEmpty($rel)) { return $false }

    $parts = $rel -split '[\\/]'
    foreach ($part in $parts) {
        if ($excludeDirNames -contains $part) { return $true }
        if ($part -eq ".cereblix") { return $true }
    }

    $leaf = Split-Path $FullPath -Leaf
    foreach ($pat in $excludeFilePatterns) {
        if ($leaf -like $pat) { return $true }
    }
    return $false
}

$stamp = Get-Date -Format "yyyyMMdd-HHmm"
$zipName = "cereblix-qt-wallet-source-$stamp.zip"
$zipPath = Join-Path $OutDir $zipName
$stageRoot = Join-Path $env:TEMP ("crb-src-stage-" + [guid]::NewGuid().ToString("n"))
$stageDir = Join-Path $stageRoot "cereblix-qt-wallet"

New-Item -ItemType Directory -Force -Path $stageDir | Out-Null
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

Write-Host "Staging source from $RepoRoot ..."
Get-ChildItem -Path $RepoRoot -Recurse -Force -File | ForEach-Object {
    if (Should-ExcludePath $_.FullName $RepoRoot) { return }
    $rel = $_.FullName.Substring($RepoRoot.Length).TrimStart('\', '/')
    $dest = Join-Path $stageDir $rel
    $destParent = Split-Path $dest -Parent
    if (-not (Test-Path $destParent)) {
        New-Item -ItemType Directory -Force -Path $destParent | Out-Null
    }
    Copy-Item $_.FullName $dest -Force
}

# Safety scan — refuse to ship wallet files
$walletHits = Get-ChildItem -Path $stageDir -Recurse -Force -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -eq "wallet.json" -or $_.Extension -eq ".wallet" }
if ($walletHits) {
    throw "Refusing to package: wallet file(s) found in staging: $($walletHits.FullName -join ', ')"
}

if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path $stageDir -DestinationPath $zipPath -CompressionLevel Optimal
Remove-Item -Recurse -Force $stageRoot

$sizeMb = [math]::Round((Get-Item $zipPath).Length / 1MB, 2)
Write-Host ""
Write-Host "Created: $zipPath ($sizeMb MB)"
Write-Host "Excluded: qt6/, build/, cereblix/, wallet files, binaries"
Write-Host "Your wallet at $env:USERPROFILE\.cereblix\wallet.json was NOT included."
