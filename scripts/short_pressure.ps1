param(
    [string]$BuildDir = "build-wsl-main"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$root = (Resolve-Path ".").Path
$scriptPath = Join-Path $root "scripts/pressure/pr_gate.ps1"

if (-not (Test-Path $scriptPath)) {
    throw "missing script: $scriptPath"
}

powershell -ExecutionPolicy Bypass -File $scriptPath -BuildDir $BuildDir

