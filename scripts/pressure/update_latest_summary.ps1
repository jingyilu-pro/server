param(
    [string]$ReportRoot = "reports/pressure",
    [Parameter(Mandatory = $true)]
    [string]$CurrentRunId,
    [Parameter(Mandatory = $true)]
    [ValidateSet("PASS", "FAILED")]
    [string]$GateStatus
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Find-ScenarioReport {
    param(
        [string]$RootPath,
        [string]$Scenario,
        [string]$RunId
    )

    $targetName = "pr_${Scenario}_${RunId}.json"
    return Get-ChildItem -Path $RootPath -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -eq $targetName } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
}

function Load-ScenarioMetric {
    param(
        [string]$RootPath,
        [string]$Scenario,
        [string]$RunId
    )

    $file = Find-ScenarioReport -RootPath $RootPath -Scenario $Scenario -RunId $RunId
    if ($null -eq $file) {
        return $null
    }

    $json = Get-Content -Path $file.FullName -Raw | ConvertFrom-Json
    $workspaceRoot = (Resolve-Path ".").Path
    $relative = $file.FullName
    if ($relative.StartsWith($workspaceRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $relative.Substring($workspaceRoot.Length).TrimStart('\\')
    }

    return [PSCustomObject]@{
        scenario = $Scenario
        file_relative = ($relative -replace '\\', '/')
        qps = [double]$json.qps
        success_rate = [double]$json.success_rate
        timeout_rate = [double]$json.timeout_rate
        p95_ms = [math]::Round(([double]$json.p95) / 1000.0, 3)
        p99_ms = [math]::Round(([double]$json.p99) / 1000.0, 3)
    }
}

function Fmt-Num {
    param([Nullable[Double]]$Value)
    if ($null -eq $Value) { return "-" }
    return ("{0:N3}" -f $Value)
}

function Fmt-Percent {
    param([Nullable[Double]]$Value)
    if ($null -eq $Value) { return "-" }
    return ("{0:N3}%" -f ($Value * 100.0))
}

if ([string]::IsNullOrWhiteSpace($CurrentRunId)) {
    throw "CurrentRunId cannot be empty"
}

$reportRootPath = Join-Path (Resolve-Path ".").Path ($ReportRoot -replace '/', '\\')
if (-not (Test-Path $reportRootPath)) {
    throw "report root missing: $reportRootPath"
}

$templatePath = Join-Path (Resolve-Path ".").Path "scripts/pressure/templates/latest_summary_template.md"
if (-not (Test-Path $templatePath)) {
    throw "latest summary template missing: $templatePath"
}

$scenarios = @("full_chain", "manager_only", "login_only", "game_only")
$scenarioRows = [System.Collections.Generic.List[string]]::new()
$scenarioMetrics = @{}
foreach ($scenario in $scenarios) {
    $metric = Load-ScenarioMetric -RootPath $reportRootPath -Scenario $scenario -RunId $CurrentRunId
    $scenarioMetrics[$scenario] = $metric
    if ($null -eq $metric) {
        $scenarioRows.Add("| ${scenario} | - | - | - | - | - |")
    }
    else {
        $scenarioRows.Add("| ${scenario} | $(Fmt-Num -Value $metric.qps) | $(Fmt-Percent -Value $metric.success_rate) | $(Fmt-Percent -Value $metric.timeout_rate) | $(Fmt-Num -Value $metric.p95_ms) | $(Fmt-Num -Value $metric.p99_ms) |")
    }
}

$compareReportFile = Get-ChildItem -Path $reportRootPath -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -eq "pr_gate_compare_${CurrentRunId}_vs_*.md" -or $_.Name -like "pr_gate_compare_${CurrentRunId}_vs_*.md" } |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1

$compareReportRelative = "-"
if ($null -ne $compareReportFile) {
    $workspaceRoot = (Resolve-Path ".").Path
    $compareReportRelative = $compareReportFile.FullName
    if ($compareReportRelative.StartsWith($workspaceRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $compareReportRelative = $compareReportRelative.Substring($workspaceRoot.Length).TrimStart('\\')
    }
    $compareReportRelative = $compareReportRelative -replace '\\', '/'
}

$reportDir = "-"
if ($null -ne $scenarioMetrics['full_chain']) {
    $reportDir = [System.IO.Path]::GetDirectoryName($scenarioMetrics['full_chain'].file_relative) -replace '\\', '/'
}

$historyPath = Join-Path $reportRootPath "latest_history.json"
$history = @()
if (Test-Path $historyPath) {
    try {
        $history = Get-Content -Path $historyPath -Raw | ConvertFrom-Json
        if ($null -eq $history) {
            $history = @()
        }
    } catch {
        $history = @()
    }
}

$history = @($history | Where-Object { $_.run_id -ne $CurrentRunId })
$fullChain = $scenarioMetrics['full_chain']
$historyItem = [PSCustomObject]@{
    run_id = $CurrentRunId
    run_time = (Get-Date -Format "yyyy-MM-dd HH:mm:ss")
    gate_status = $GateStatus
    full_chain_success_rate = $(if ($null -eq $fullChain) { $null } else { [double]$fullChain.success_rate })
    full_chain_timeout_rate = $(if ($null -eq $fullChain) { $null } else { [double]$fullChain.timeout_rate })
    full_chain_p95_ms = $(if ($null -eq $fullChain) { $null } else { [double]$fullChain.p95_ms })
    compare_report = $compareReportRelative
}

$history = @($historyItem) + @($history)
if ($history.Count -gt 20) {
    $history = @($history | Select-Object -First 20)
}

$history | ConvertTo-Json -Depth 6 | Set-Content -Path $historyPath -Encoding UTF8

$historyRows = [System.Collections.Generic.List[string]]::new()
foreach ($item in $history) {
    $historyRows.Add("| $($item.run_id) | $($item.run_time) | $($item.gate_status) | $(Fmt-Percent -Value $item.full_chain_success_rate) | $(Fmt-Percent -Value $item.full_chain_timeout_rate) | $(Fmt-Num -Value $item.full_chain_p95_ms) | $($item.compare_report) |")
}

$template = Get-Content -Path $templatePath -Raw -Encoding UTF8
$template = $template.Replace("{{RUN_ID}}", $CurrentRunId)
$template = $template.Replace("{{RUN_TIME}}", (Get-Date -Format "yyyy-MM-dd HH:mm:ss"))
$template = $template.Replace("{{GATE_STATUS}}", $GateStatus)
$template = $template.Replace("{{REPORT_DIR}}", $reportDir)
$template = $template.Replace("{{COMPARE_REPORT}}", $compareReportRelative)
$template = $template.Replace("{{SCENARIO_ROWS}}", ($scenarioRows -join "`n"))
$template = $template.Replace("{{HISTORY_ROWS}}", ($historyRows -join "`n"))

$latestPath = Join-Path $reportRootPath "latest_summary.md"
[System.IO.File]::WriteAllText($latestPath, $template, [System.Text.Encoding]::UTF8)

Write-Host "[pr-gate] latest summary updated: $($latestPath -replace '\\', '/')"
exit 0
