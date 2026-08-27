#!/usr/bin/env bash
#
# validate-examples.sh — prove that every runnable example in examples/ is
# actually correct: compiles warning-free under the skill's own T1 warning
# baseline (references/tooling.md) and runs clean under AddressSanitizer +
# UndefinedBehaviorSanitizer with all asserts enabled.
#
# Why this exists: the skill's rule is "no bad code examples, ever." The docs
# link each behavioral rule to a demo_*() function in examples/; this script
# is the gate that keeps those demos honest. If an example is wrong, the repo
# fails its own build.
#
# Platform: bash on Linux/macOS. Windows: run under WSL — the examples
#           exercise POSIX APIs (pipes, sigaction, mkdtemp) that native
#           Windows toolchains don't provide.
# Dependencies: a C compiler with sanitizer support — clang (default `cc` on
#           macOS) or gcc. Override with e.g. `CC=gcc-14 scripts/validate-examples.sh`.
#
# Usage:  scripts/validate-examples.sh
# Exit:   0 if every example compiles and passes, 1 otherwise.
set -u

# Resolve the repo root from this script's own location.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# Honor $CC if the caller set one; default to the system compiler.
CC="${CC:-cc}"

# Build artifacts go in a private temp dir, removed on exit (even on failure).
OUT_DIR="${TMPDIR:-/tmp}/expert-c-examples.$$"
mkdir -p "$OUT_DIR"
trap 'rm -rf "$OUT_DIR"' EXIT

# The flag set, mirroring the skill's own guidance:
#  - line 1-3: the T1 warning baseline (tooling.md) — warnings are errors
#  - line 4:   the T2 dev/test build — ASan+UBSan, and -fno-sanitize-recover
#              so any UB finding aborts (fails the run) instead of just logging
#  - -fno-omit-frame-pointer keeps sanitizer stack traces readable
#  - -pthread: examples/undefined_behavior.c demonstrates mutex-protected
#              sharing; the flag is required on older glibc/musl and harmless
#              everywhere else
FLAGS=(
    -std=c17 -Wall -Wextra -Werror
    -Wformat=2 -Wconversion -Wsign-conversion -Wshadow
    -Wimplicit-fallthrough -Wvla -Wdouble-promotion
    -g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all
    -fno-omit-frame-pointer
    -pthread
)

FAIL=0
for src in "$ROOT"/examples/*.c; do
    name="$(basename "$src" .c)"
    bin="$OUT_DIR/$name"

    # Step 1: compile. Any warning is an error (-Werror), so a mere diagnostic
    # in an example is enough to fail the build.
    if ! "$CC" "${FLAGS[@]}" -o "$bin" "$src" 2>"$OUT_DIR/$name.err"; then
        echo "COMPILE FAIL: $name"
        cat "$OUT_DIR/$name.err"
        FAIL=1
        continue
    fi

    # Step 2: run. The demos assert their rules' claimed behavior; a failed
    # assert, an ASan finding, or a UBSan finding all yield nonzero exit.
    # Run inside OUT_DIR so demos that create scratch files (e.g. mkdtemp in
    # security.c) never litter the repo.
    if ! (cd "$OUT_DIR" && "./$name" >"$OUT_DIR/$name.out" 2>&1); then
        echo "RUN FAIL: $name"
        cat "$OUT_DIR/$name.out"
        FAIL=1
        continue
    fi

    echo "PASS: $name — $(cat "$OUT_DIR/$name.out")"
done

if [ "$FAIL" -eq 0 ]; then
    echo "OK: all examples compile warning-free and run clean under ASan+UBSan."
else
    echo "Example validation FAILED."
fi
exit "$FAIL"
