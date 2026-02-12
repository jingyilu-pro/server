param(
    [string]$BuildDir = "build-wsl-main",
    [string]$ReportRoot = "reports/pressure",
    [double]$TargetTimeoutRate = 0.05,
    [int]$MaxRounds = 6,
    [int]$MySqlCoroWorkers = 4,
    [int]$RedisCoroWorkers = 2,
    [int]$MySqlPasswordHashIterations = 20000,
    [double]$MinSuccessRate = 0.995,
    [string]$Scenario = "full_chain",
    [int]$WarmupSec = 10,
    [int]$DurationSec = 90,
    [int]$VirtualUsers = 20,
    [int]$TargetRps = 80,
    [int]$RampUpSec = 5,
    [int]$TimeoutMs = 1000,
    [int]$AccountPoolSize = 160
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function New-AutoTuneYaml {
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
        [int]$MySqlCoroWorkers,
        [int]$RedisCoroWorkers,
        [int]$MySqlPasswordHashIterations
    )

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

redis:
  host: 127.0.0.1
  port: 6379
  coro_workers: $RedisCoroWorkers

mysql:
  host: 127.0.0.1
  port: 3306
  user: game_app
  password: ""
  database: game
  connect_timeout_ms: 2000
  coro_workers: $MySqlCoroWorkers
  password_hash_iterations: $MySqlPasswordHashIterations

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
  report:
    interval_sec: 2
    output: json
    output_dir: $OutputDir
    prefix: $Prefix
    json_path: $OutputDir/$Prefix.json
  http:
    coro_workers: 1
"@ | Set-Content -Path $Path -Encoding ASCII
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

$durationSec = [Math]::Max(10, $DurationSec)
$virtualUsers = [Math]::Max(1, $VirtualUsers)
$targetRps = [Math]::Max(1, $TargetRps)
$rampUpSec = [Math]::Max(0, $RampUpSec)
$timeoutMs = [Math]::Max(100, $TimeoutMs)
$accountPoolSize = [Math]::Max([Math]::Max(4, $virtualUsers), $AccountPoolSize)

$success = $false

for ($round = 1; $round -le $MaxRounds; $round++) {
    $prefix = "autotune_${Scenario}_r${round}_$runId"
    $configName = "scripts/pressure/generated.autotune.r${round}.$runId.yaml"
    $configPath = Join-Path (Resolve-Path ".").Path $configName
    $reportPath = Join-Path $reportDirWindows "$prefix.json"

    try {
        New-AutoTuneYaml -Path $configPath `
            -Scenario $Scenario `
            -WarmupSec $WarmupSec `
            -DurationSec $durationSec `
            -VirtualUsers $virtualUsers `
            -TargetRps $targetRps `
            -RampUpSec $rampUpSec `
            -TimeoutMs $timeoutMs `
            -AccountPoolSize $accountPoolSize `
            -OutputDir $reportDirRelative `
            -Prefix $prefix `
            -MySqlCoroWorkers $MySqlCoroWorkers `
            -RedisCoroWorkers $RedisCoroWorkers `
            -MySqlPasswordHashIterations $MySqlPasswordHashIterations

        $configPathWsl = "/mnt/c/Work/Projects/server/$($configName.Replace('\\','/'))"
        $binaryWsl = "./$BuildDir/app/application/application"
        $timeoutSec = [Math]::Max(25, $durationSec + 30)
        $cmd = "cd /mnt/c/Work/Projects/server && timeout ${timeoutSec}s $binaryWsl --mode all --config $configPathWsl"

        Write-Host "[autotune] scenario=$Scenario round=$round run with vus=$virtualUsers rps=$targetRps timeout_ms=$timeoutMs"
        wsl bash -lc $cmd | Out-Host
        if ($LASTEXITCODE -ne 0 -and $LASTEXITCODE -ne 124) {
            throw "autotune process failed at round=$round, exit_code=$LASTEXITCODE"
        }

        if (-not (Test-Path $reportPath)) {
            throw "autotune report missing: $reportPath"
        }

        $data = Get-Content $reportPath -Raw | ConvertFrom-Json
        $timeoutRate = [double]$data.timeout_rate
        $successRate = [double]$data.success_rate
        $p95Ms = [math]::Round(([double]$data.p95) / 1000.0, 3)
        $guardStopped = $false
        if ($data.PSObject.Properties.Name -contains 'early_stopped_by_timeout_guard') {
            $guardStopped = [bool]$data.early_stopped_by_timeout_guard
        }

        Write-Host "[autotune] scenario=$Scenario round=$round timeout_rate=$timeoutRate success_rate=$successRate p95_ms=$p95Ms guard=$guardStopped"

        if ($timeoutRate -le $TargetTimeoutRate -and $successRate -ge $MinSuccessRate) {
            $success = $true
            Write-Host "[autotune] scenario=$Scenario target reached at round=$round"
            break
        }

        $targetRps = [Math]::Max(5, [int][Math]::Floor($targetRps * 0.7))
        $virtualUsers = [Math]::Max(5, [int][Math]::Floor($virtualUsers * 0.85))
        $timeoutMs = [Math]::Min(4000, $timeoutMs + 200)
        $accountPoolSize = [Math]::Max($accountPoolSize, [int]($virtualUsers * 4))
    }
    finally {
        Remove-Item $configPath -Force -ErrorAction SilentlyContinue
    }
}

if (-not $success) {
    Write-Error "[autotune] failed to satisfy timeout<=${TargetTimeoutRate} and success_rate>=${MinSuccessRate} in $MaxRounds rounds"
    exit 1
}

exit 0
