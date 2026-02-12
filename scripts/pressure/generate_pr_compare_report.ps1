param(
    [string]$ReportRoot = "reports/pressure",
    [Parameter(Mandatory = $true)]
    [string]$CurrentRunId
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Cn {
    param([int[]]$Codes)
    return -join ($Codes | ForEach-Object { [char]$_ })
}

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
        return [PSCustomObject]@{
            scenario = $Scenario
            exists = $false
            file_relative = ""
            qps = $null
            success_rate = $null
            timeout_rate = $null
            p95_ms = $null
            p99_ms = $null
            timeout_guard = $null
        }
    }

    $json = Get-Content -Path $file.FullName -Raw | ConvertFrom-Json
    $workspaceRoot = (Resolve-Path ".").Path
    $relative = $file.FullName
    if ($relative.StartsWith($workspaceRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $relative.Substring($workspaceRoot.Length).TrimStart('\\')
    }

    return [PSCustomObject]@{
        scenario = $Scenario
        exists = $true
        file_relative = ($relative -replace '\\', '/')
        qps = [double]$json.qps
        success_rate = [double]$json.success_rate
        timeout_rate = [double]$json.timeout_rate
        p95_ms = [math]::Round(([double]$json.p95) / 1000.0, 3)
        p99_ms = [math]::Round(([double]$json.p99) / 1000.0, 3)
        timeout_guard = [bool]$json.early_stopped_by_timeout_guard
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

function Fmt-Delta {
    param(
        [Nullable[Double]]$NewValue,
        [Nullable[Double]]$OldValue,
        [bool]$AsPercentPoint = $false
    )

    if ($null -eq $NewValue -or $null -eq $OldValue) {
        return "-"
    }

    $delta = $NewValue - $OldValue
    if ($AsPercentPoint) {
        return ("{0}{1:N3}pp" -f ($(if ($delta -ge 0) { "+" } else { "" })), ($delta * 100.0))
    }

    return ("{0}{1:N3}" -f ($(if ($delta -ge 0) { "+" } else { "" })), $delta)
}

if ([string]::IsNullOrWhiteSpace($CurrentRunId)) {
    throw "CurrentRunId cannot be empty"
}

$reportRootPath = Join-Path (Resolve-Path ".").Path ($ReportRoot -replace '/', '\\')
if (-not (Test-Path $reportRootPath)) {
    Write-Host "[pr-gate] report root missing, skip compare report"
    exit 0
}

$fullChainFiles = Get-ChildItem -Path $reportRootPath -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object { $_.Name -match '^pr_full_chain_(\d{14}_[0-9a-fA-F]+)\.json$' }
if ($fullChainFiles.Count -eq 0) {
    Write-Host "[pr-gate] no full_chain history, skip compare report"
    exit 0
}

$runIds = $fullChainFiles |
    ForEach-Object {
        if ($_.Name -match '^pr_full_chain_(\d{14}_[0-9a-fA-F]+)\.json$') {
            $Matches[1]
        }
    } |
    Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
    Sort-Object -Unique |
    Sort-Object -Descending

$previousRunId = $runIds | Where-Object { $_ -ne $CurrentRunId } | Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($previousRunId)) {
    Write-Host "[pr-gate] no previous run id, skip compare report"
    exit 0
}

$scenarios = @("full_chain", "manager_only", "login_only", "game_only")
$currentMetrics = @{}
$previousMetrics = @{}
foreach ($scenario in $scenarios) {
    $currentMetrics[$scenario] = Load-ScenarioMetric -RootPath $reportRootPath -Scenario $scenario -RunId $CurrentRunId
    $previousMetrics[$scenario] = Load-ScenarioMetric -RootPath $reportRootPath -Scenario $scenario -RunId $previousRunId
}

$currentFullChain = Find-ScenarioReport -RootPath $reportRootPath -Scenario "full_chain" -RunId $CurrentRunId
if ($null -ne $currentFullChain) {
    $outputDir = $currentFullChain.DirectoryName
}
else {
    $outputDir = Join-Path $reportRootPath (Get-Date -Format "yyyy-MM-dd")
    if (-not (Test-Path $outputDir)) {
        New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
    }
}

$templatePath = Join-Path (Resolve-Path ".").Path "scripts/pressure/templates/pr_compare_template.md"
if (-not (Test-Path $templatePath)) {
    throw "compare template missing: $templatePath"
}

$labelCurrent = Cn @(0x5F53,0x524D)
$labelPrevious = Cn @(0x4E0A,0x6B21)
$fullColon = [char]0xFF1A

$coreRows = [System.Collections.Generic.List[string]]::new()
$deltaRows = [System.Collections.Generic.List[string]]::new()
$rawRows = [System.Collections.Generic.List[string]]::new()

foreach ($scenario in $scenarios) {
    $cur = $currentMetrics[$scenario]
    $pre = $previousMetrics[$scenario]

    if ($cur.exists) {
        $coreRows.Add("| ${scenario} | ${labelCurrent} | $(Fmt-Num -Value $cur.qps) | $(Fmt-Percent -Value $cur.success_rate) | $(Fmt-Percent -Value $cur.timeout_rate) | $(Fmt-Num -Value $cur.p95_ms) | $(Fmt-Num -Value $cur.p99_ms) | $($cur.timeout_guard) |")
        $rawRows.Add("- ${labelCurrent} ${scenario}${fullColon} $($cur.file_relative)")
    }
    else {
        $coreRows.Add("| ${scenario} | ${labelCurrent} | - | - | - | - | - | - |")
    }

    if ($pre.exists) {
        $coreRows.Add("| ${scenario} | ${labelPrevious} | $(Fmt-Num -Value $pre.qps) | $(Fmt-Percent -Value $pre.success_rate) | $(Fmt-Percent -Value $pre.timeout_rate) | $(Fmt-Num -Value $pre.p95_ms) | $(Fmt-Num -Value $pre.p99_ms) | $($pre.timeout_guard) |")
        $rawRows.Add("- ${labelPrevious} ${scenario}${fullColon} $($pre.file_relative)")
    }
    else {
        $coreRows.Add("| ${scenario} | ${labelPrevious} | - | - | - | - | - | - |")
    }

    $deltaRows.Add("| ${scenario} | $(Fmt-Delta -NewValue $cur.qps -OldValue $pre.qps) | $(Fmt-Delta -NewValue $cur.success_rate -OldValue $pre.success_rate -AsPercentPoint $true) | $(Fmt-Delta -NewValue $cur.timeout_rate -OldValue $pre.timeout_rate -AsPercentPoint $true) | $(Fmt-Delta -NewValue $cur.p95_ms -OldValue $pre.p95_ms) | $(Fmt-Delta -NewValue $cur.p99_ms -OldValue $pre.p99_ms) |")
}

$content = Get-Content -Path $templatePath -Raw -Encoding UTF8
$content = $content.Replace('{{CURRENT_RUN_ID}}', $CurrentRunId)
$content = $content.Replace('{{PREVIOUS_RUN_ID}}', $previousRunId)
$content = $content.Replace('{{GENERATED_AT}}', (Get-Date -Format "yyyy-MM-dd HH:mm:ss"))
$content = $content.Replace('{{CORE_ROWS}}', ($coreRows -join "`n"))
$content = $content.Replace('{{DELTA_ROWS}}', ($deltaRows -join "`n"))
$content = $content.Replace('{{RAW_ROWS}}', ($rawRows -join "`n"))

$outputPath = Join-Path $outputDir "pr_gate_compare_${CurrentRunId}_vs_${previousRunId}.md"
[System.IO.File]::WriteAllText($outputPath, $content, [System.Text.Encoding]::UTF8)

Write-Host "[pr-gate] compare report generated: $($outputPath -replace '\\', '/')"
exit 0
