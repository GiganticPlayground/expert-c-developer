# Secure C: Untrusted Input, Buffers, Integers

These rules are distilled chiefly from the SEI CERT C Coding Standard. Assume
every extern-facing byte is hostile. Pair these rules with the hardening flags
in [tooling.md](tooling.md).

---

## SEC1. Bounds-check every write; the length always travels with the buffer

**Why:** buffer overflow remains the canonical C vulnerability; any write
whose size isn't provably ≤ remaining capacity is a finding.

```c
/* BAD: no relation between input length and buffer */
char name[32];
strcpy(name, user_input);

/* GOOD: capacity-aware, truncation detected */
char name[32];
int n = snprintf(name, sizeof name, "%s", user_input);
if (n < 0 || (size_t)n >= sizeof name) { return -ENAMETOOLONG; }
```

APIs you design must take `(buf, len)` pairs — a bare `char *` out-parameter
is an unfinished interface.

**Runnable:** [`demo_sec1()` in examples/security.c](../examples/security.c)

**Source:**
[CERT STR31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/characters-and-strings-str/str31-c/);
[CERT ARR30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/arrays-arr/arr30-c/);
[CERT ARR38-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/arrays-arr/arr38-c/)
(library calls must not form invalid pointers).

---

## SEC2. Banned functions: `gets`, `strcpy`/`strcat`/`sprintf` on untrusted data

**Why:** they cannot be used safely with attacker-controlled input — no length
parameter (`gets` was removed from the language in C11 for this reason).

Replacements: `fgets` for line input; `snprintf` for bounded formatting and
copying; `memcpy` with an explicitly computed, checked length.

**Runnable:** [`demo_sec2()` in examples/security.c](../examples/security.c)

**Source:**
[CERT MSC24-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/miscellaneous-msc/msc24-c/)
(obsolescent functions);
[CERT STR31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/characters-and-strings-str/str31-c/);
[cppreference: gets (removed in C11)](https://en.cppreference.com/w/c/io/gets).

---

## SEC3. Format strings are code — never let input become one

**Why:** `%n` and friends turn a printf family call with a user-controlled
format into arbitrary read/write.

```c
/* BAD: classic format-string vulnerability */
printf(user_msg);
syslog(LOG_INFO, user_msg);

/* GOOD: constant format, data as argument */
printf("%s", user_msg);
```

`-Wformat=2` flags non-literal formats (see [tooling.md](tooling.md)).

**Runnable:** [`demo_sec3()` in examples/security.c](../examples/security.c)

**Source:**
[CERT FIO30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/input-output-fio/fio30-c/);
[OpenSSF Hardening Guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html)
(`-Wformat=2`).

---

## SEC4. Integer checks come BEFORE the operation that needs them

**Why:** overflowed sizes buy undersized buffers (see
[memory.md M3](memory.md)); overflowed offsets bypass bounds checks. Signed
overflow is UB, so after-the-fact tests get optimized away
([gotchas.md G10](gotchas.md)).

Checklist for any arithmetic on sizes/offsets/counts from outside:
- addition: `a > LIMIT - b` before `a + b`;
- multiplication: `a > LIMIT / b` before `a * b` (or `calloc`);
- narrowing casts: range-check before casting;
- signed/unsigned mixes: normalize the domain first
  ([gotchas.md G5](gotchas.md)).

**Runnable:** [`demo_sec4()` in examples/security.c](../examples/security.c)

**Source:**
[CERT INT32-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int32-c/);
[CERT INT30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int30-c/);
[CERT INT31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int31-c/)
(integer conversions must not lose data);
[CERT MEM35-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem35-c/).

---

## SEC5. Never build shell commands from input — avoid `system()` entirely

**Why:** `system` runs a shell: metacharacters in any interpolated string
become command injection, and the executed binary resolves via a
caller-controlled `PATH`.

```c
/* BAD: filename = "x; rm -rf ~" */
char cmd[256];
snprintf(cmd, sizeof cmd, "gzip %s", filename);
system(cmd);
```

Use `fork`+`execve` with an argument vector (no shell parsing), validating the
argument against an allowlist.

**Runnable:** [`demo_sec5()` in examples/security.c](../examples/security.c)
(demonstrates the allowlist half; the exec-vector half is intentionally not
spawned in tests)

**Source:**
[CERT ENV33-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/environment-env/env33-c/).

---

## SEC6. Filesystem: don't check-then-use (TOCTOU)

**Why:** between `access()`/`stat()` and the `open()`, an attacker swaps the
file for a symlink. The check and the use must be one atomic operation.

```c
/* BAD: race between access() and fopen() */
if (access(path, W_OK) == 0) {
    fp = fopen(path, "w");
}

/* GOOD: open with the constraints expressed to the kernel, then use the fd */
int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
if (fd < 0) { return -1; }
```

Note the semantics: `O_EXCL` is the create-*new* pattern and fails if the file
exists. For updating a possibly-existing file, open with `O_NOFOLLOW` (no
`O_EXCL`) and validate the opened fd with `fstat` (type, owner) before
writing — the checks move onto the fd, where they can't race.

**Runnable:** [`demo_sec6()` in examples/security.c](../examples/security.c)

**Source:**
[CERT FIO45-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/input-output-fio/fio45-c/).

---

## SEC7. Secrets: never hard-code, and erase in a way the optimizer can't elide

**Why:** credentials in the binary are extractable with `strings`; a plain
`memset` before `free` is dead-store-eliminated because the memory is never
read again.

```c
/* BAD: optimizer deletes the memset */
memset(password, 0, sizeof password);
free(password);
```

Use `memset_explicit` (C23), `explicit_bzero` (BSD/glibc), or
`SecureZeroMemory` (Windows); load secrets from the environment/secret store,
not literals. Where none of those APIs exist, the portable fallback is a write
loop through a `volatile unsigned char *` — that is what the runnable demo
uses, since `memset_explicit` isn't yet universal.

**Runnable:** [`demo_sec7()` in examples/security.c](../examples/security.c)

**Source:**
[CERT MSC41-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/miscellaneous-msc/msc41-c/)
(no hard-coded sensitive information);
[CERT MSC06-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/miscellaneous-msc/msc06-c/)
(compiler optimization vs. sensitive data);
[cppreference: memset_explicit](https://en.cppreference.com/w/c/string/byte/memset).

---

## SEC8. `rand()` is never a security primitive

**Why:** libc `rand` is a small-state deterministic generator — predictable
seeds and outputs; unusable for tokens, keys, nonces, or anything an attacker
benefits from guessing.

Use the platform CSPRNG: `getrandom(2)` (Linux), `arc4random_buf` (BSD/macOS),
`BCryptGenRandom` (Windows). `rand`/`random` remain fine for simulations and
tests.

**Runnable:** [`demo_sec8()` in examples/security.c](../examples/security.c)

**Source:**
[CERT MSC30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/miscellaneous-msc/msc30-c/).

---

## SEC9. Ship with hardening flags on

**Why:** compiler mitigations (stack protectors, `_FORTIFY_SOURCE`, PIE/ASLR,
RELRO, CFI) turn many residual bugs from exploitable into crash-only — but
only if they're enabled in the release build.

The current flag set, per platform and compiler version, is maintained in the
[OpenSSF Compiler Options Hardening Guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html);
our distilled baseline lives in [tooling.md](tooling.md).

**Runnable:** the warning/sanitizer baseline is exercised on every example build by [scripts/validate-examples.sh](../scripts/validate-examples.sh)

**Source:**
[OpenSSF Compiler Options Hardening Guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html).
