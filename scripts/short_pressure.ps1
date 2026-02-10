param(
    [string]$BuildDir = "build-wsl-main",
    [string]$ConfigPath = "all.yaml"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$tmpConfig = Join-Path (Resolve-Path ".").Path "scripts/short_pressure.generated.yaml"
@"
server:
  manager:
    host: 127.0.0.1
    port: 18080
  login:
    host: 127.0.0.1
    port: 18081
  game:
    host: 127.0.0.1
    port: 18082

client_pressure:
  enabled: true
  target:
    discovery_role: manager
  scenario:
    duration_sec: 8
    virtual_users: 4
    target_rps: 16
    ramp_up_sec: 2
    request_timeout_ms: 1200
    auto_relogin: true
    login_account_pool:
      - smoke_user_01
      - smoke_user_02
      - smoke_user_03
      - smoke_user_04
  report:
    interval_sec: 2
    output: log
    json_path: short_pressure_report.json
"@ | Set-Content -Path $tmpConfig -Encoding ASCII

Write-Host "[pressure] config=$tmpConfig"
$configPathWsl = "/mnt/c/Work/Projects/server/scripts/short_pressure.generated.yaml"
$cmd = "cd /mnt/c/Work/Projects/server && timeout 12s ./build-wsl-main/app/application/application --mode all --config $configPathWsl"
wsl bash -lc $cmd | Out-Host

if (Test-Path $tmpConfig) {
    Remove-Item $tmpConfig -Force
}

Write-Host "[pressure] done"
