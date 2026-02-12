param(
    [int]$DurationSec = 120,
    [int]$IntervalSec = 1,
    [string]$OutputPath = "reports/pressure/tcp_state_monitor_latest.csv",
    [string]$Ports = "18080"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Invoke-WslQuiet {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BashCommand
    )

    $previousErrorAction = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & wsl.exe -d Ubuntu-24.04 -- bash -lc $BashCommand 2>$null
        if ($LASTEXITCODE -ne 0 -or $null -eq $output) {
            return @()
        }
        return $output
    }
    finally {
        $ErrorActionPreference = $previousErrorAction
    }
}

if ($DurationSec -le 0) {
    throw "DurationSec 必须 > 0"
}
if ($IntervalSec -le 0) {
    throw "IntervalSec 必须 > 0"
}

$outDir = Split-Path -Parent $OutputPath
if (-not [string]::IsNullOrWhiteSpace($outDir) -and -not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$portList = @()
foreach ($raw in $Ports.Split(',')) {
    $item = $raw.Trim()
    if ([string]::IsNullOrWhiteSpace($item)) {
        continue
    }

    [int]$parsed = 0
    if (-not [int]::TryParse($item, [ref]$parsed) -or $parsed -le 0 -or $parsed -gt 65535) {
        throw "非法端口: $item"
    }
    $portList += $parsed
}

$deadline = (Get-Date).AddSeconds($DurationSec)
$rows = New-Object System.Collections.Generic.List[object]

Write-Host "[tcp-monitor] 开始采样: duration=${DurationSec}s interval=${IntervalSec}s ports=$Ports"

while ((Get-Date) -lt $deadline) {
    $sampleTs = Get-Date
    $sampleIso = $sampleTs.ToString("s")

    $sockStat = Invoke-WslQuiet -BashCommand "cat /proc/net/sockstat"
    $timewait = 0
    $estab = 0
    foreach ($line in $sockStat) {
        if ($line -match "^TCP:\s+inuse\s+(\d+)\s+orphan\s+\d+\s+tw\s+(\d+)") {
            $estab = [int]$Matches[1]
            $timewait = [int]$Matches[2]
            break
        }
    }

    $record = [ordered]@{
        timestamp = $sampleIso
        estab_total = $estab
        timewait_total = $timewait
    }

    foreach ($port in $portList) {
        $portTimeWaitOutput = Invoke-WslQuiet -BashCommand "ss -tan state time-wait | grep -E '(:$port )|(:$port$)' | wc -l"
        $portEstabOutput = Invoke-WslQuiet -BashCommand "ss -tan state established | grep -E '(:$port )|(:$port$)' | wc -l"

        $portTimeWait = "$portTimeWaitOutput".Trim()
        $portEstab = "$portEstabOutput".Trim()

        [int]$portTwNum = 0
        [int]$portEsNum = 0
        [void][int]::TryParse($portTimeWait, [ref]$portTwNum)
        [void][int]::TryParse($portEstab, [ref]$portEsNum)

        $record["timewait_$port"] = $portTwNum
        $record["established_$port"] = $portEsNum
    }

    $rows.Add([PSCustomObject]$record)
    Start-Sleep -Seconds $IntervalSec
}

$rows | Export-Csv -Path $OutputPath -NoTypeInformation -Encoding UTF8

$maxTw = ($rows | Measure-Object -Property timewait_total -Maximum).Maximum
$avgTw = [math]::Round((($rows | Measure-Object -Property timewait_total -Average).Average), 2)

Write-Host "[tcp-monitor] 输出文件: $OutputPath"
Write-Host "[tcp-monitor] TIME_WAIT total max=$maxTw avg=$avgTw"
