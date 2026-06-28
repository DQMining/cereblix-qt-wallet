# Download cereblixd from https://github.com/CereblixCRB/cereblix/releases
param(
    [string]$Tag = "v2.4.0",
    [ValidateSet("windows", "linux-amd64", "linux-arm64")]
    [string]$Platform = "windows",
    [Parameter(Mandatory = $true)]
    [string]$OutFile
)

$ErrorActionPreference = "Stop"

$Assets = @{
    windows       = "cereblixd-windows-amd64.exe"
    "linux-amd64" = "cereblixd-linux-amd64"
    "linux-arm64" = "cereblixd-linux-arm64"
}

$asset = $Assets[$Platform]
$url = "https://github.com/CereblixCRB/cereblix/releases/download/$Tag/$asset"

$outDir = Split-Path $OutFile -Parent
if ($outDir -and -not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

Write-Host "Fetching cereblixd $Tag ($asset) ..."
Invoke-WebRequest -Uri $url -OutFile $OutFile -UseBasicParsing

if (-not (Test-Path $OutFile) -or (Get-Item $OutFile).Length -lt 1024) {
    throw "Download failed or file too small: $OutFile"
}

Write-Host "Saved: $OutFile ($([math]::Round((Get-Item $OutFile).Length / 1MB, 2)) MB)"
