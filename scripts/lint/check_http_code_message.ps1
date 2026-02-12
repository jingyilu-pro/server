param(
    [string[]]$TargetFiles = @(
        'app/service/base/basic_http_service.cpp',
        'app/service/manager/common/manager_service.cpp',
        'app/service/login/common/login_service.cpp',
        'app/service/game/common/game_service.cpp'
    )
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$rules = @(
    [PSCustomObject]@{ Name = 'no numeric set_code'; Pattern = '\bset_code\s*\(\s*\d+\s*\)' },
    [PSCustomObject]@{ Name = 'no literal set_message'; Pattern = '\bset_message\s*\(\s*"' },
    [PSCustomObject]@{ Name = 'no numeric evhttp_send_error status'; Pattern = '\bevhttp_send_error\s*\(\s*[^,]+,\s*\d+\s*,' },
    [PSCustomObject]@{ Name = 'no literal evhttp_send_error text'; Pattern = '\bevhttp_send_error\s*\(\s*[^,]+,\s*[^,]+,\s*"' }
)

$violations = @()

foreach ($relativePath in $TargetFiles) {
    if (-not (Test-Path $relativePath)) {
        continue
    }

    $lineNo = 0
    foreach ($line in Get-Content -Path $relativePath) {
        $lineNo++
        foreach ($rule in $rules) {
            if ($line -match $rule.Pattern) {
                $violations += [PSCustomObject]@{
                    file = $relativePath
                    line = $lineNo
                    rule = $rule.Name
                    snippet = $line.Trim()
                }
            }
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Host "Found code/message hardcoding violations: $($violations.Count)" -ForegroundColor Red
    foreach ($item in $violations) {
        Write-Host "- [$($item.rule)] $($item.file):$($item.line)" -ForegroundColor Red
        Write-Host "  $($item.snippet)"
    }
    exit 1
}

Write-Host 'Check passed: no code/message hardcoding found.' -ForegroundColor Green
exit 0

