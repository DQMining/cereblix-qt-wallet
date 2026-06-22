param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

$WalletExe = Join-Path $RepoRoot "qt-wallet\build\$Config\cereblix-qt-wallet.exe"
$CliWallet = Join-Path $RepoRoot "cereblix\cereblix-wallet.exe"

if (-not (Test-Path $WalletExe)) {
    throw "Qt wallet not built: $WalletExe"
}

Write-Host "=== Qt self-test ==="
& $WalletExe --self-test | Out-Host
$selfTestCode = $LASTEXITCODE
if ($null -ne $selfTestCode -and $selfTestCode -ne 0) { exit $selfTestCode }

if (-not (Test-Path $CliWallet)) {
    Write-Host "Skipping CLI cross-test (cereblix-wallet.exe not found)"
    Write-Host "CRYPTO VALIDATE PASS"
    exit 0
}

$TempDir = Join-Path $env:TEMP ("crb-wallet-test-" + [guid]::NewGuid().ToString("n"))
New-Item -ItemType Directory -Path $TempDir | Out-Null
$WalletPath = Join-Path $TempDir "wallet.json"
$Pass = "test-pass-1234"

try {
    Write-Host "=== CLI create + encrypt ==="
    & $CliWallet -wallet $WalletPath new main 2>&1 | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "CLI new failed" }

    $env:CEREBRA_PASSPHRASE = $Pass
    & $CliWallet -wallet $WalletPath encrypt 2>&1 | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "CLI encrypt failed" }

    $json = Get-Content $WalletPath -Raw | ConvertFrom-Json
    if (-not $json.encrypted) { throw "Wallet file is not marked encrypted" }
    if ($json.kdf -ne "pbkdf2-sha256") { throw "Unexpected KDF: $($json.kdf)" }

    Write-Host "=== Qt unlock CLI-encrypted wallet ==="
    & $WalletExe --unlock-wallet $WalletPath 2>&1 | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "Qt could not unlock CLI-encrypted wallet" }

    Write-Host "CLI/Qt encrypted wallet cross-compatibility OK"
    Write-Host "CRYPTO VALIDATE PASS"
}
finally {
    Remove-Item -Recurse -Force $TempDir -ErrorAction SilentlyContinue
    Remove-Item Env:CEREBRA_PASSPHRASE -ErrorAction SilentlyContinue
}
