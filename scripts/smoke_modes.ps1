param(
    [string]$BuildDir = "build-wsl-main",
    [string]$ConfigPath = "all.yaml"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$modes = @("manager", "login", "game", "client", "all")

foreach ($mode in $modes) {
    Write-Host "[smoke] mode=$mode"
    $cmd = "cd /mnt/c/Work/Projects/server && timeout 5s ./build-wsl-main/app/application/application --mode $mode --config $ConfigPath"
    wsl bash -lc $cmd | Out-Host
}

Write-Host "[smoke] done"

