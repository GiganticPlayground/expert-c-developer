#!/usr/bin/env bash
#
# check-links.sh — verify that every external URL cited in this skill resolves.
#
# Why this exists: the skill's core promise is that every rule cites its
# source with a full URL. A dead link breaks that promise silently, so this
# script makes link rot a build failure instead.
#
# Design notes:
#  - URLs are checked IN PARALLEL (8 workers) with short timeouts and one
#    retry, so the whole sweep finishes in ~1 minute instead of stalling for
#    20s x 3 on every unreachable host.
#  - SOFT_HOSTS lists hosts that are known to rate-limit or block datacenter
#    and burst traffic (c-faq.com runs on a 2005-era server that drops cloud
#    IPs). Failures there are reported as WARN and do NOT fail the build —
#    otherwise CI would permafail on a host that works fine from a browser.
#    Verify WARN'd links manually once in a while.
#
# Platform: bash on Linux/macOS. Windows users: run under WSL or Git Bash,
#           or use the native PowerShell port, scripts/check-links.ps1.
# Dependencies: curl, xargs, plus standard Unix tools (grep, sed, sort).
#
# Usage:  scripts/check-links.sh        (from anywhere; paths are computed)
# Exit:   0 if every hard URL resolves (WARNs allowed), 1 otherwise.
set -u

# Resolve the repo root from this script's own location, so the script works
# no matter what directory it is invoked from.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# Hosts that are checked but never fail the build (see design notes above).
# Extended-regex alternation, e.g. 'c-faq\.com|example\.org'.
export SOFT_HOSTS='c-faq\.com'

# Collect every http(s) URL from all Markdown files:
#  - grep -o extracts just the URL, not the whole line
#  - the character class stops at whitespace, ')', '"' and '>' so Markdown
#    link syntax doesn't leak into the URL
#  - sed strips trailing punctuation that prose leaves attached (".", ";", ...)
#  - URLs containing "/OWNER/" are documentation placeholders, not real links
#  - sort -u dedupes so each URL is checked exactly once
urls=$(grep -rhoE 'https?://[^ )">]+' "$ROOT"/*.md "$ROOT"/references/*.md \
    | sed -E 's/[).,;]+$//' | grep -v '/OWNER/' | sort -u)

# Check one URL; print "OK ...", "WARN ...", or "FAIL ..." on a single line.
# Two attempts with a 12s timeout each and a 2s pause between — enough to
# absorb a transient hiccup without letting a dead host stall the sweep.
# A curl code of 000 means the connection itself failed (timeout, DNS, reset).
check_one() {
    url="$1"
    code="000"
    for attempt in 1 2; do
        code=$(curl -s -o /dev/null -w "%{http_code}" -L --max-time 12 \
            -A "Mozilla/5.0 (link-check; expert-c-developer skill)" "$url")
        case "$code" in
            2*|3*) echo "OK $url"; return 0 ;;   # success or redirect
        esac
        sleep 2
    done
    if printf '%s' "$url" | grep -qE "$SOFT_HOSTS"; then
        echo "WARN $code $url"                    # known bot-hostile host
    else
        echo "FAIL $code $url"
    fi
}
export -f check_one

# Fan the URL list across 8 parallel workers. Each result is one short echo,
# so interleaved output stays line-intact in practice.
results=$(printf '%s\n' "$urls" | xargs -P 8 -I{} bash -c 'check_one "$@"' _ {})

total=$(printf '%s\n' "$urls" | grep -c .)
fails=$(printf '%s\n' "$results" | grep -c '^FAIL' || true)
warns=$(printf '%s\n' "$results" | grep -c '^WARN' || true)

# Show anything that wasn't a clean OK.
printf '%s\n' "$results" | grep -E '^(FAIL|WARN)' || true

if [ "$fails" -eq 0 ]; then
    echo "OK: $((total - warns))/$total URLs resolve ($warns warn-only, from soft hosts)."
    exit 0
else
    echo "Link check FAILED: $fails dead of $total URLs ($warns warn-only)."
    exit 1
fi
