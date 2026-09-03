param(
    [switch]$StrictGeneratedFiles
)

$ErrorActionPreference = "Stop"

$Repos = @(
    [pscustomobject]@{
        Name = "WargameEngine"
        Path = Join-Path $env:USERPROFILE "Documents\Codex\WargameEngine"
    },
    [pscustomobject]@{
        Name = "OpenVic"
        Path = Join-Path $env:USERPROFILE "Documents\Codex\OpenVic-OurMotor"
    },
    [pscustomobject]@{
        Name = "OpenVic-Simulation"
        Path = Join-Path $env:USERPROFILE "Documents\Codex\OpenVic-OurMotor\extension\deps\openvic-simulation"
    }
)

$overallPass = $true

foreach ($repo in $Repos) {
    Write-Host ""
    Write-Host "=== $($repo.Name) ===" -ForegroundColor Cyan

    if (-not (Test-Path (Join-Path $repo.Path ".git"))) {
        Write-Host "FAIL: not a Git checkout at $($repo.Path)" -ForegroundColor Red
        $overallPass = $false
        continue
    }

    Push-Location $repo.Path
    try {
        git fetch --all --prune --quiet

        $branch = (git branch --show-current).Trim()
        $head = (git rev-parse HEAD).Trim()

        $upstream = ""
        try {
            $upstream = (git rev-parse --abbrev-ref --symbolic-full-name '@{u}' 2>$null).Trim()
        } catch {}

        Write-Host "Path:     $($repo.Path)"
        Write-Host "Branch:   $branch"
        Write-Host "HEAD:     $head"

        if ([string]::IsNullOrWhiteSpace($upstream)) {
            Write-Host "FAIL: no upstream configured." -ForegroundColor Red
            $overallPass = $false
        } else {
            $upstreamHead = (git rev-parse $upstream).Trim()
            Write-Host "Upstream: $upstream"
            Write-Host "Remote:   $upstreamHead"

            $counts = (git rev-list --left-right --count "$head...$upstreamHead").Trim() -split "\s+"
            $ahead = [int]$counts[0]
            $behind = [int]$counts[1]

            Write-Host "Ahead:    $ahead"
            Write-Host "Behind:   $behind"

            if ($ahead -ne 0 -or $behind -ne 0) {
                Write-Host "FAIL: local HEAD does not exactly match upstream HEAD." -ForegroundColor Red
                $overallPass = $false
            } else {
                Write-Host "HEAD MATCH: PASS" -ForegroundColor Green
            }
        }

        $status = @(git status --porcelain=v1)
        $trackedDirty = @()
        $generated = @()
        $otherUntracked = @()

        foreach ($line in $status) {
            if ($line.Length -lt 4) { continue }
            $code = $line.Substring(0,2)
            $path = $line.Substring(3).Trim()

            if ($code -ne "??") {
                $trackedDirty += $line
                continue
            }

            if (
                $path -match '(^|/)build[^/]*/?$' -or
                $path -match '(^|/)build-foundation-[^/]*/?$' -or
                $path -match '(^|/)build-reconciliation-[^/]*/?$'
            ) {
                $generated += $line
            } else {
                $otherUntracked += $line
            }
        }

        if ($trackedDirty.Count -gt 0) {
            Write-Host "FAIL: tracked working-tree changes:" -ForegroundColor Red
            $trackedDirty | ForEach-Object { Write-Host "  $_" }
            $overallPass = $false
        } else {
            Write-Host "Tracked tree: PASS" -ForegroundColor Green
        }

        if ($otherUntracked.Count -gt 0) {
            Write-Host "FAIL: non-generated untracked files:" -ForegroundColor Red
            $otherUntracked | ForEach-Object { Write-Host "  $_" }
            $overallPass = $false
        }

        if ($generated.Count -gt 0) {
            Write-Host "Generated/untracked build output:" -ForegroundColor Yellow
            $generated | ForEach-Object { Write-Host "  $_" }
            if ($StrictGeneratedFiles) {
                $overallPass = $false
            }
        }
    }
    finally {
        Pop-Location
    }
}

Write-Host ""
if ($overallPass) {
    Write-Host "PROJECT GITHUB SYNC: PASS" -ForegroundColor Green
    exit 0
}

Write-Host "PROJECT GITHUB SYNC: FAIL" -ForegroundColor Red
exit 1