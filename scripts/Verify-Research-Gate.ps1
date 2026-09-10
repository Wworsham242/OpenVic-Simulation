param(
    [switch]$Staged
)

$ErrorActionPreference = "Stop"

$repo = git rev-parse --show-toplevel
if ($LASTEXITCODE -ne 0) {
    throw "Not inside a Git repository."
}

Set-Location $repo

$requiredFiles = @(
    "AGENTS.md",
    "docs/architecture/REAL-WORLD-MODEL-REFERENCE-FRAMEWORK.md"
)

foreach ($file in $requiredFiles) {
    if (-not (Test-Path $file)) {
        throw "RESEARCH GATE FAILED: required project guardrail missing: $file"
    }
}

$agents = Get-Content "AGENTS.md" -Raw
$framework =
    Get-Content `
        "docs/architecture/REAL-WORLD-MODEL-REFERENCE-FRAMEWORK.md" `
        -Raw

if (
    $agents -notmatch
    'REAL-WORLD-MODEL-REFERENCE-FRAMEWORK\.md'
) {
    throw "RESEARCH GATE FAILED: AGENTS.md does not require the reference framework."
}

if (
    $framework -notmatch
    'DARPA World Modelers'
) {
    throw "RESEARCH GATE FAILED: World Modelers reference missing."
}

if (
    $framework -notmatch
    'International Futures'
) {
    throw "RESEARCH GATE FAILED: IFs reference missing."
}

if (
    $framework -notmatch
    'GCAM'
) {
    throw "RESEARCH GATE FAILED: GCAM reference missing."
}

if (
    $framework -notmatch
    'JTLS-GO'
) {
    throw "RESEARCH GATE FAILED: JTLS-GO reference missing."
}

if (
    $framework -notmatch
    'Causal Exploration'
) {
    throw "RESEARCH GATE FAILED: Causal Exploration reference missing."
}

if ($Staged) {
    $changed = @(
        git diff --cached --name-only --diff-filter=ACMR
    )
} else {
    $changed = @(
        git diff --name-only --diff-filter=ACMR
    )

    if ($LASTEXITCODE -ne 0) {
        throw "Unable to inspect tracked worktree changes."
    }

    $changed += @(
        git diff --cached --name-only --diff-filter=ACMR
    )

    if ($LASTEXITCODE -ne 0) {
        throw "Unable to inspect staged changes."
    }

    # git diff does not report untracked files. New convergence documents
    # must still pass the research gate before they are staged.
    $changed += @(
        git ls-files --others --exclude-standard
    )

    if ($LASTEXITCODE -ne 0) {
        throw "Unable to inspect untracked files."
    }

    $changed = @(
        $changed |
        Sort-Object -Unique
    )
}

if ($LASTEXITCODE -ne 0) {
    throw "Unable to inspect changed files."
}

$architectureDocs = @(
    $changed |
    Where-Object {
        $_ -match '^docs/architecture/PROJECT-CONVERGENCE-.*\.md$'
    }
)

# Existing/certified documents are grandfathered. Only enforce the new
# section template on documents that actually contain the marker below.
#
# New work created under this framework should contain:
#
#   Research-Gate-Version: 1
#
# This avoids retroactively blocking historical convergence commits.

$requiredSections = @(
    "## Native Repository Basis",
    "## External Reference Review",
    "## Chosen Mechanism",
    "## Calibration Status",
    "## Causal Integration",
    "## Scope Boundary"
)

foreach ($doc in $architectureDocs) {
    if (-not (Test-Path $doc)) {
        continue
    }

    $text = Get-Content $doc -Raw

    if ($text -notmatch 'Research-Gate-Version:\s*1') {
        continue
    }

    foreach ($section in $requiredSections) {
        if (-not $text.Contains($section)) {
            throw (
                "RESEARCH GATE FAILED: " +
                "$doc is missing required section '$section'"
            )
        }
    }
}

Write-Host "Research gate: PASS"
Write-Host "Mandatory framework present and intact."

if ($architectureDocs.Count -gt 0) {
    Write-Host ""
    Write-Host "Convergence documents examined:"
    $architectureDocs | ForEach-Object {
        Write-Host "  $_"
    }
}

exit 0
