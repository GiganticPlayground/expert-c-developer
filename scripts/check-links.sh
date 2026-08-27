#!/usr/bin/env bash
#
# check-links.sh — verify that every external URL cited in this skill resolves.
#
# Why this exists: the skill's core promise is that every rule cites its
# source with a full URL. A dead link breaks that promise silently, so this
# script makes link rot a build failure instead.
#
# Platform: bash on Linux/macOS. Windows users: run under WSL or Git Bash,
#           or use the native PowerShell port, scripts/check-links.ps1.
# Dependencies: curl, plus standard Unix tools (grep, sed, sort).
#
# Usage:  scripts/check-links.sh        (from anywhere; paths are computed)
# Exit:   0 if every URL resolves, 1 if any URL failed after retries.
set -u

# Resolve the repo root from this script's own location, so the script works
# no matter what directory it is invoked from.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FAIL=0

# Collect every http(s) URL from all Markdown files:
#  - grep -o extracts just the URL, not the whole line
#  - the character class stops at whitespace, ')', '"' and '>' so Markdown
#    link syntax doesn't leak into the URL
#  - sed strips trailing punctuation that prose leaves attached (".", ";", ...)
#  - URLs containing "/OWNER/" are documentation placeholders (e.g. the
#    clone command in README.md), not real links — skip them
#  - sort -u dedupes so each URL is checked exactly once
urls=$(grep -rhoE 'https?://[^ )">]+' "$ROOT"/*.md "$ROOT"/references/*.md \
    | sed -E 's/[).,;]+$//' | grep -v '/OWNER/' | sort -u)

# Fetch one URL and print its final HTTP status code.
#  -s silent, -o /dev/null discard body, -L follow redirects,
#  -A sets a User-Agent (some sites reject requests without one).
# A code of 000 means the connection itself failed (timeout, DNS, reset).
fetch_code() {
    curl -s -o /dev/null -w "%{http_code}" -L --max-time 20 \
        -A "Mozilla/5.0 (link-check; expert-c-developer skill)" "$1"
}

total=0
while IFS= read -r url; do
    total=$((total + 1))

    # Up to 3 attempts per URL with a short growing delay between them.
    # Old or rate-limited hosts (looking at you, c-faq.com) drop connections
    # under burst load and recover seconds later — retries separate genuinely
    # dead links from transient network noise.
    ok=0
    for attempt in 1 2 3; do
        code=$(fetch_code "$url")
        case "$code" in
            2*|3*) ok=1; break ;;              # success or redirect: link is alive
            *)     sleep $((attempt * 2)) ;;   # transient? back off, retry
        esac
    done

    if [ "$ok" -eq 0 ]; then
        echo "FAIL $code $url"
        FAIL=1
    fi
done <<< "$urls"

if [ "$FAIL" -eq 0 ]; then
    echo "OK: all $total unique URLs resolve."
else
    echo "Link check FAILED (of $total unique URLs)."
fi
exit "$FAIL"
