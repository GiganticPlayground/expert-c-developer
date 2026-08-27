# check-links.ps1 — native Windows (PowerShell) port of check-links.sh.
#
# Verifies that every external URL cited in the skill's Markdown resolves.
# Functionally equivalent to the bash version: extracts unique http(s) URLs,
# skips /OWNER/ placeholders, retries transient failures, exits nonzero on
# any dead link.
#
# Requirements: PowerShell 5.1+ (Windows) or PowerShell 7+ (any OS).
#               No external dependencies — uses Invoke-WebRequest.
# Usage:        powershell -ExecutionPolicy Bypass -File scripts\check-links.ps1
#
# NOTE: the example validator has no native Windows port — the examples
# exercise POSIX APIs (pipes, sigaction, mkdtemp). Use WSL for
# validate-examples.sh; this link checker is fully native.

$ErrorActionPreference = 'Continue'

# Repo root, computed from this script's own location.
$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

# Hosts that are checked but never fail the build: known to rate-limit or
# block datacenter/burst traffic (c-faq.com runs on a 2005-era server).
# Failures there are reported as WARN. Mirrors SOFT_HOSTS in check-links.sh.
$SoftHosts = 'c-faq\.com'

# Collect every unique http(s) URL from all Markdown files, trimming the
# trailing punctuation Markdown prose leaves attached, skipping placeholders.
$files = @(Get-ChildItem -Path $Root -Filter '*.md') +
         @(Get-ChildItem -Path (Join-Path $Root 'references') -Filter '*.md')
$urls = $files |
    Select-String -Pattern 'https?://[^ )">]+' -AllMatches |
    ForEach-Object { $_.Matches.Value } |
    ForEach-Object { $_ -replace '[).,;]+$', '' } |
    Where-Object { $_ -notmatch '/OWNER/' } |
    Sort-Object -Unique

$failed = 0
$total = 0
foreach ($url in $urls) {
    $total++
    $ok = $false

    # Two attempts with a short delay — old/rate-limited hosts drop burst
    # connections and recover seconds later.
    for ($attempt = 1; $attempt -le 2; $attempt++) {
        try {
            $resp = Invoke-WebRequest -Uri $url -Method Head -MaximumRedirection 5 `
                -TimeoutSec 20 -UserAgent 'Mozilla/5.0 (link-check; expert-c-developer skill)' `
                -UseBasicParsing
            if ($resp.StatusCode -ge 200 -and $resp.StatusCode -lt 400) { $ok = $true; break }
        } catch {
            # Some servers reject HEAD; fall back to GET before counting a failure.
            try {
                $resp = Invoke-WebRequest -Uri $url -Method Get -MaximumRedirection 5 `
                    -TimeoutSec 20 -UserAgent 'Mozilla/5.0 (link-check; expert-c-developer skill)' `
                    -UseBasicParsing
                if ($resp.StatusCode -ge 200 -and $resp.StatusCode -lt 400) { $ok = $true; break }
            } catch { }
        }
        Start-Sleep -Seconds ($attempt * 2)
    }

    if (-not $ok) {
        if ($url -match $SoftHosts) {
            Write-Output "WARN $url (soft host: does not fail the check)"
        } else {
            Write-Output "FAIL $url"
            $failed = 1
        }
    }
}

if ($failed -eq 0) {
    Write-Output "OK: all $total unique URLs resolve."
} else {
    Write-Output "Link check FAILED (of $total unique URLs)."
}
exit $failed
