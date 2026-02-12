param(
    [string]$BuildDir = "build-wsl-main",
    [string]$ReportRoot = "reports/pressure"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function New-PressureYaml {
    param(
        [string]$Path,
        [string]$Scenario,
        [int]$WarmupSec,
        [int]$DurationSec,
        [int]$VirtualUsers,
        [int]$TargetRps,
        [int]$RampUpSec,
        [int]$TimeoutMs,
        [int]$AccountPoolSize,
        [string]$OutputDir,
        [string]$Prefix,
        [string]$RedisKeyPrefix
    )

    @"
server:
  manager:
    host: 127.0.0.1
    port: 18080
  login:
    host: 127.0.0.1
    port: 0
  game:
    host: 127.0.0.1
    port: 0

client_pressure:
  enabled: true
  target:
    discovery_role: manager
  scenario:
    scenario: $Scenario
    warmup_sec: $WarmupSec
    duration_sec: $DurationSec
    virtual_users: $VirtualUsers
    target_rps: $TargetRps
    ramp_up_sec: $RampUpSec
    timeout_ms: $TimeoutMs
    account_pool_size: $AccountPoolSize
    auto_relogin: true
  guard:
    enabled: true
    min_samples: 100
    min_success_rate: 0.995
    max_timeout_rate: 0.005
    max_p95_ms: 150
    max_p99_ms: 300
  report:
    interval_sec: 10
    output: json
    output_dir: $OutputDir
    prefix: $Prefix
    json_path: $OutputDir/$Prefix.json
  http:
    coro_workers: 1

redis:
  host: 127.0.0.1
  port: 6379
  key_prefix: $RedisKeyPrefix
"@ | Set-Content -Path $Path -Encoding ASCII
}

function Invoke-OneScenario {
    param(
        [string]$Scenario,
        [int]$WarmupSec,
        [int]$DurationSec,
        [int]$VirtualUsers,
        [int]$TargetRps,
        [int]$RampUpSec,
        [int]$TimeoutMs,
        [int]$AccountPoolSize,
        [string]$ReportDirRelative,
        [string]$RunId,
        [string]$Label
    )

    $configName = "scripts/pressure/generated.weekly.$Label.$RunId.yaml"
    $configPath = Join-Path (Resolve-Path ".").Path $configName
    $reportPrefix = "weekly_${Label}_$RunId"
    $redisKeyPrefix = "svc_${RunId}_${Label}_${Scenario}"

    New-PressureYaml -Path $configPath `
        -Scenario $Scenario `
        -WarmupSec $WarmupSec `
        -DurationSec $DurationSec `
        -VirtualUsers $VirtualUsers `
        -TargetRps $TargetRps `
        -RampUpSec $RampUpSec `
        -TimeoutMs $TimeoutMs `
        -AccountPoolSize $AccountPoolSize `
        -OutputDir $ReportDirRelative `
        -Prefix $reportPrefix `
        -RedisKeyPrefix $redisKeyPrefix

    $configPathWsl = "/mnt/c/Work/Projects/server/$($configName.Replace('\\','/'))"
    $binaryWsl = "./$BuildDir/app/application/application"
    $timeoutSec = [Math]::Max(30, $WarmupSec + $DurationSec + 30)
    $cmd = "cd /mnt/c/Work/Projects/server && timeout ${timeoutSec}s $binaryWsl --mode all --config $configPathWsl"

    Write-Host "[weekly] run label=$Label scenario=$Scenario"
    wsl bash -lc $cmd | Out-Host
    if ($LASTEXITCODE -eq 124) {
        Write-Host "[weekly] label=$Label scenario=$Scenario reached timeout window (expected in --mode all)"
    } elseif ($LASTEXITCODE -ne 0) {
        throw "pressure process failed, scenario=$Scenario, label=$Label, exit_code=$LASTEXITCODE"
    }

    Remove-Item $configPath -Force -ErrorAction SilentlyContinue
}

$dateDir = Get-Date -Format "yyyy-MM-dd"
$reportRootRelative = (($ReportRoot -replace '\\', '/') -replace '/+', '/').TrimEnd('/')
$reportDirRelative = "$reportRootRelative/$dateDir"
$reportDirWindows = Join-Path (Resolve-Path ".").Path ($reportDirRelative -replace '/', '\\')
if (-not (Test-Path $reportDirWindows)) {
    New-Item -ItemType Directory -Force -Path $reportDirWindows | Out-Null
}

$sha = (git rev-parse --short HEAD).Trim()
if ([string]::IsNullOrWhiteSpace($sha)) {
    $sha = "nogit"
}
$runId = "$(Get-Date -Format 'yyyyMMddHHmmss')_$sha"

Invoke-OneScenario -Scenario "full_chain" -Label "steady" -WarmupSec 120 -DurationSec 3300 -VirtualUsers 150 -TargetRps 700 -RampUpSec 120 -TimeoutMs 1000 -AccountPoolSize 600 -ReportDirRelative $reportDirRelative -RunId $runId
Invoke-OneScenario -Scenario "full_chain" -Label "burst" -WarmupSec 30 -DurationSec 900 -VirtualUsers 250 -TargetRps 1200 -RampUpSec 60 -TimeoutMs 1200 -AccountPoolSize 1000 -ReportDirRelative $reportDirRelative -RunId $runId
Invoke-OneScenario -Scenario "full_chain" -Label "recovery" -WarmupSec 30 -DurationSec 1200 -VirtualUsers 100 -TargetRps 450 -RampUpSec 60 -TimeoutMs 1000 -AccountPoolSize 400 -ReportDirRelative $reportDirRelative -RunId $runId

Invoke-OneScenario -Scenario "manager_only" -Label "manager" -WarmupSec 30 -DurationSec 600 -VirtualUsers 120 -TargetRps 1000 -RampUpSec 30 -TimeoutMs 300 -AccountPoolSize 480 -ReportDirRelative $reportDirRelative -RunId $runId
Invoke-OneScenario -Scenario "login_only" -Label "login" -WarmupSec 30 -DurationSec 600 -VirtualUsers 80 -TargetRps 350 -RampUpSec 30 -TimeoutMs 1000 -AccountPoolSize 320 -ReportDirRelative $reportDirRelative -RunId $runId
Invoke-OneScenario -Scenario "game_only" -Label "game" -WarmupSec 30 -DurationSec 600 -VirtualUsers 120 -TargetRps 700 -RampUpSec 30 -TimeoutMs 600 -AccountPoolSize 480 -ReportDirRelative $reportDirRelative -RunId $runId

Write-Host "[weekly] done"
