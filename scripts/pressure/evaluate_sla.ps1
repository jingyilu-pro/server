param(
    [Parameter(Mandatory = $true)]
    [string]$ReportJson,
    [Parameter(Mandatory = $true)]
    [string]$Scenario,
    [double]$MinSuccessRate = 0,
    [double]$MaxTimeoutRate = 1,
    [double]$MaxP95Ms = 1.0E12,
    [double]$MaxP99Ms = 1.0E12
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path $ReportJson)) {
    Write-Error "missing report file: $ReportJson"
    exit 2
}

$data = Get-Content $ReportJson -Raw | ConvertFrom-Json

$successRate = [double]$data.success_rate
$timeoutRate = [double]$data.timeout_rate
$p95Us = [double]$data.p95
$p99Us = [double]$data.p99

$p95Ms = $p95Us / 1000.0
$p99Ms = $p99Us / 1000.0

$pass = $true

if ($successRate -lt $MinSuccessRate) {
    Write-Host "[sla][FAIL] scenario=$Scenario success_rate=$successRate < $MinSuccessRate"
    $pass = $false
}
if ($timeoutRate -gt $MaxTimeoutRate) {
    Write-Host "[sla][FAIL] scenario=$Scenario timeout_rate=$timeoutRate > $MaxTimeoutRate"
    $pass = $false
}
if ($p95Ms -gt $MaxP95Ms) {
    Write-Host "[sla][FAIL] scenario=$Scenario p95_ms=$p95Ms > $MaxP95Ms"
    $pass = $false
}
if ($p99Ms -gt $MaxP99Ms) {
    Write-Host "[sla][FAIL] scenario=$Scenario p99_ms=$p99Ms > $MaxP99Ms"
    $pass = $false
}

if ($pass) {
    Write-Host "[sla][PASS] scenario=$Scenario success_rate=$successRate timeout_rate=$timeoutRate p95_ms=$p95Ms p99_ms=$p99Ms"
    exit 0
}

exit 1

