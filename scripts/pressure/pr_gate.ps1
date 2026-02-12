param(
    [string]$BuildDir = "build-wsl-main",
    [string]$ReportRoot = "reports/pressure",
    [switch]$EnableAutoTuneFallback = $true,
    [double]$AutoTuneTargetTimeoutRate = 0.05,
    [double]$AutoTuneMinSuccessRate = 0.995,
    [int]$AutoTuneMaxRounds = 6,
    [int]$MySqlCoroWorkers = 4,
    [int]$RedisCoroWorkers = 4,
    [int]$MySqlPasswordHashIterations = 12000
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function New-PressureYaml {
    param(
        [string]$Path,
        [int]$ManagerPort,
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
        [string]$RedisKeyPrefix,
        [int]$MySqlCoroWorkers,
        [int]$RedisCoroWorkers,
        [int]$MySqlPasswordHashIterations,
        [double]$GuardMinSuccessRate,
        [double]$GuardMaxTimeoutRate,
        [double]$GuardMaxP95Ms,
        [double]$GuardMaxP99Ms
    )

    @"
server:
  manager:
    host: 127.0.0.1
    port: $ManagerPort
  login:
    host: 127.0.0.1
    port: 0
  game:
    host: 127.0.0.1
    port: 0

redis:
  host: 127.0.0.1
  port: 6379
  key_prefix: $RedisKeyPrefix
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
  guard:
    enabled: true
    min_samples: 100
    min_success_rate: $GuardMinSuccessRate
    max_timeout_rate: $GuardMaxTimeoutRate
    max_p95_ms: $GuardMaxP95Ms
    max_p99_ms: $GuardMaxP99Ms
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

function Invoke-OnePressure {
    param(
        [string]$Scenario,
        [int]$ManagerPort,
        [int]$WarmupSec,
        [int]$DurationSec,
        [int]$VirtualUsers,
        [int]$TargetRps,
        [int]$RampUpSec,
        [int]$TimeoutMs,
        [int]$AccountPoolSize,
        [double]$MinSuccessRate,
        [double]$MaxTimeoutRate,
        [double]$MaxP95Ms,
        [double]$MaxP99Ms,
        [string]$ReportDirRelative,
        [string]$ReportDirWindows,
        [string]$RunId
    )

    $configName = "scripts/pressure/generated.pr.$Scenario.$RunId.yaml"
    $configPath = Join-Path (Resolve-Path ".").Path $configName
    $reportPrefix = "pr_${Scenario}_$RunId"
    $redisKeyPrefix = "svc_${RunId}_${Scenario}"

    New-PressureYaml -Path $configPath `
        -ManagerPort $ManagerPort `
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
        -RedisKeyPrefix $redisKeyPrefix `
        -MySqlCoroWorkers $MySqlCoroWorkers `
        -RedisCoroWorkers $RedisCoroWorkers `
        -MySqlPasswordHashIterations $MySqlPasswordHashIterations `
        -GuardMinSuccessRate $MinSuccessRate `
        -GuardMaxTimeoutRate $MaxTimeoutRate `
        -GuardMaxP95Ms $MaxP95Ms `
        -GuardMaxP99Ms $MaxP99Ms

    $configPathWsl = "/mnt/c/Work/Projects/server/$($configName.Replace('\\','/'))"
    $binaryWsl = "./$BuildDir/app/application/application"
    $timeoutSec = [Math]::Max(20, $WarmupSec + $DurationSec + 15)
    $cmd = "cd /mnt/c/Work/Projects/server && timeout ${timeoutSec}s $binaryWsl --mode all --config $configPathWsl"

    try {
        Write-Host "[pr-gate] run scenario=$Scenario config=$configName"
        wsl bash -lc $cmd | Out-Host
        if ($LASTEXITCODE -eq 124) {
            Write-Host "[pr-gate] scenario=$Scenario reached timeout window (expected in --mode all)"
        } elseif ($LASTEXITCODE -ne 0) {
            throw "pressure process failed, scenario=$Scenario, exit_code=$LASTEXITCODE"
        }

        $reportPath = Join-Path $ReportDirWindows "$reportPrefix.json"
        & powershell -ExecutionPolicy Bypass -File scripts/pressure/evaluate_sla.ps1 `
            -ReportJson $reportPath `
            -Scenario $Scenario `
            -MinSuccessRate $MinSuccessRate `
            -MaxTimeoutRate $MaxTimeoutRate `
            -MaxP95Ms $MaxP95Ms `
            -MaxP99Ms $MaxP99Ms
        if ($LASTEXITCODE -ne 0) {
            $json = Get-Content $reportPath -Raw | ConvertFrom-Json
            $actualTimeoutRate = [double]$json.timeout_rate
            $result = [ordered]@{
                scenario = $Scenario
                report_path = $reportPath
                timeout_rate = $actualTimeoutRate
                pass = $false
                reason = "sla_failed"
            }
            return [PSCustomObject]$result
        }

        $passResult = [ordered]@{
            scenario = $Scenario
            report_path = $reportPath
            timeout_rate = [double](Get-Content $reportPath -Raw | ConvertFrom-Json).timeout_rate
            pass = $true
            reason = "pass"
        }

        return [PSCustomObject]$passResult
    }
    finally {
        Remove-Item $configPath -Force -ErrorAction SilentlyContinue
    }
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

$failed = $false

function Invoke-ScenarioWithOptionalAutoTune {
    param(
        [string]$Scenario,
        [int]$ManagerPort,
        [int]$WarmupSec,
        [int]$DurationSec,
        [int]$VirtualUsers,
        [int]$TargetRps,
        [int]$RampUpSec,
        [int]$TimeoutMs,
        [int]$AccountPoolSize,
        [double]$MinSuccessRate,
        [double]$MaxTimeoutRate,
        [double]$MaxP95Ms,
        [double]$MaxP99Ms,
        [string]$ReportDirRelative,
        [string]$ReportDirWindows,
        [string]$RunId
    )

    $result = Invoke-OnePressure -Scenario $Scenario -ManagerPort $ManagerPort -WarmupSec $WarmupSec -DurationSec $DurationSec -VirtualUsers $VirtualUsers -TargetRps $TargetRps -RampUpSec $RampUpSec -TimeoutMs $TimeoutMs -AccountPoolSize $AccountPoolSize -MinSuccessRate $MinSuccessRate -MaxTimeoutRate $MaxTimeoutRate -MaxP95Ms $MaxP95Ms -MaxP99Ms $MaxP99Ms -ReportDirRelative $ReportDirRelative -ReportDirWindows $ReportDirWindows -RunId $RunId

    if ($result -is [System.Array]) {
        $result = $result | Where-Object {
            $_ -is [PSCustomObject] -and ($_.PSObject.Properties.Name -contains "pass")
        } | Select-Object -Last 1
    }

    if ($null -eq $result -or -not ($result.PSObject.Properties.Name -contains "pass")) {
        throw "invalid pressure result object, scenario=$Scenario"
    }

    if ($result.pass) {
        return $true
    }

    Write-Host "[pr-gate] scenario=$Scenario SLA failed, timeout_rate=$($result.timeout_rate)"
    if (-not $EnableAutoTuneFallback) {
        return $false
    }
    if ($result.timeout_rate -le 0.05) {
        return $false
    }

    Write-Host "[pr-gate] scenario=$Scenario trigger auto-tune fallback (timeout_rate > 5%)"
    & powershell -ExecutionPolicy Bypass -File scripts/pressure/auto_tune_timeout.ps1 `
        -BuildDir $BuildDir `
        -ReportRoot $ReportRoot `
        -ManagerPort $ManagerPort `
        -TargetTimeoutRate $AutoTuneTargetTimeoutRate `
        -MinSuccessRate $AutoTuneMinSuccessRate `
        -MaxRounds $AutoTuneMaxRounds `
        -MySqlCoroWorkers $MySqlCoroWorkers `
        -RedisCoroWorkers $RedisCoroWorkers `
        -MySqlPasswordHashIterations $MySqlPasswordHashIterations `
        -Scenario $Scenario `
        -WarmupSec $WarmupSec `
        -DurationSec $DurationSec `
        -VirtualUsers $VirtualUsers `
        -TargetRps $TargetRps `
        -RampUpSec $RampUpSec `
        -TimeoutMs $TimeoutMs `
        -AccountPoolSize $AccountPoolSize

    return ($LASTEXITCODE -eq 0)
}

try {
    if (-not (Invoke-ScenarioWithOptionalAutoTune -Scenario "full_chain" -ManagerPort (Get-Random -Minimum 20080 -Maximum 20999) -WarmupSec 30 -DurationSec 300 -VirtualUsers 40 -TargetRps 200 -RampUpSec 30 -TimeoutMs 800 -AccountPoolSize 160 -MinSuccessRate 0.995 -MaxTimeoutRate 0.005 -MaxP95Ms 150 -MaxP99Ms 300 -ReportDirRelative $reportDirRelative -ReportDirWindows $reportDirWindows -RunId $runId)) {
        throw "full_chain failed"
    }
} catch {
    $failed = $true
    Write-Host "[pr-gate] full_chain failed: $($_.Exception.Message)"
}

try {
    if (-not (Invoke-ScenarioWithOptionalAutoTune -Scenario "manager_only" -ManagerPort (Get-Random -Minimum 21000 -Maximum 21999) -WarmupSec 20 -DurationSec 120 -VirtualUsers 50 -TargetRps 400 -RampUpSec 20 -TimeoutMs 300 -AccountPoolSize 200 -MinSuccessRate 0.999 -MaxTimeoutRate 0.002 -MaxP95Ms 40 -MaxP99Ms 80 -ReportDirRelative $reportDirRelative -ReportDirWindows $reportDirWindows -RunId $runId)) {
        throw "manager_only failed"
    }
} catch {
    $failed = $true
    Write-Host "[pr-gate] manager_only failed: $($_.Exception.Message)"
}

try {
    if (-not (Invoke-ScenarioWithOptionalAutoTune -Scenario "login_only" -ManagerPort (Get-Random -Minimum 22000 -Maximum 22999) -WarmupSec 20 -DurationSec 120 -VirtualUsers 30 -TargetRps 150 -RampUpSec 20 -TimeoutMs 800 -AccountPoolSize 120 -MinSuccessRate 0.997 -MaxTimeoutRate 0.003 -MaxP95Ms 120 -MaxP99Ms 250 -ReportDirRelative $reportDirRelative -ReportDirWindows $reportDirWindows -RunId $runId)) {
        throw "login_only failed"
    }
} catch {
    $failed = $true
    Write-Host "[pr-gate] login_only failed: $($_.Exception.Message)"
}

try {
    if (-not (Invoke-ScenarioWithOptionalAutoTune -Scenario "game_only" -ManagerPort (Get-Random -Minimum 23000 -Maximum 23999) -WarmupSec 20 -DurationSec 120 -VirtualUsers 50 -TargetRps 250 -RampUpSec 20 -TimeoutMs 500 -AccountPoolSize 200 -MinSuccessRate 0.998 -MaxTimeoutRate 0.003 -MaxP95Ms 80 -MaxP99Ms 180 -ReportDirRelative $reportDirRelative -ReportDirWindows $reportDirWindows -RunId $runId)) {
        throw "game_only failed"
    }
} catch {
    $failed = $true
    Write-Host "[pr-gate] game_only failed: $($_.Exception.Message)"
}

if ($failed) {
    try {
        & powershell -ExecutionPolicy Bypass -File scripts/pressure/generate_pr_compare_report.ps1 `
            -ReportRoot $ReportRoot `
            -CurrentRunId $runId
    } catch {
        Write-Warning "[pr-gate] 生成对比报告失败: $($_.Exception.Message)"
    }
    try {
        & powershell -ExecutionPolicy Bypass -File scripts/pressure/update_latest_summary.ps1 `
            -ReportRoot $ReportRoot `
            -CurrentRunId $runId `
            -GateStatus FAILED
    } catch {
        Write-Warning "[pr-gate] update latest summary failed: $($_.Exception.Message)"
    }

    Write-Error "[pr-gate] FAILED"
    exit 1
}

try {
    & powershell -ExecutionPolicy Bypass -File scripts/pressure/generate_pr_compare_report.ps1 `
        -ReportRoot $ReportRoot `
        -CurrentRunId $runId
} catch {
    Write-Warning "[pr-gate] 生成对比报告失败: $($_.Exception.Message)"
}

try {
    & powershell -ExecutionPolicy Bypass -File scripts/pressure/update_latest_summary.ps1 `
        -ReportRoot $ReportRoot `
        -CurrentRunId $runId `
        -GateStatus PASS
} catch {
    Write-Warning "[pr-gate] update latest summary failed: $($_.Exception.Message)"
}

Write-Host "[pr-gate] PASS"
exit 0
