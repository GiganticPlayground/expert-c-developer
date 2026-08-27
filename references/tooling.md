# Tooling: Warnings, Sanitizers, Hardening, Enforcement

The rule behind every rule:
[Power of Ten, Rule 10](https://spinroot.com/gerard/pdf/P10.pdf) — all
warnings on, all available tools, from day one, at zero warnings. Primary
flag authority:
[OpenSSF Compiler Options Hardening Guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html)
(actively maintained — consult it for the current full table; the baselines
below are the distilled minimum).

---

## T1. Warning baseline — the default for all new code

```sh
-std=c17 -Wall -Wextra -Werror \
-Wformat=2 -Wconversion -Wsign-conversion -Wshadow \
-Wimplicit-fallthrough -Wvla -Wdouble-promotion
```

- The explicit exceptions: on **legacy code** the minimum is
  `-Wall -Wextra -Werror`, with the rest phased in; `-Werror` is CI-mandatory
  but locally negotiable. Warnings never land either way.
- `-Wformat=2` catches non-literal format strings
  ([security.md SEC3](security.md)).
- `-Wconversion`/`-Wsign-conversion` catch the signed/unsigned traps
  ([gotchas.md G5](gotchas.md)) — noisy on legacy code, mandatory on new code.
- `-Wvla` enforces the no-VLA rule
  ([Power of Ten, Rule 3](https://spinroot.com/gerard/pdf/P10.pdf);
  [BARR-C](https://barrgroup.com/embedded-c-coding-standard)).

**Source:**
[OpenSSF Hardening Guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html);
[GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html);
[Power of Ten, Rule 10](https://spinroot.com/gerard/pdf/P10.pdf).

---

## T2. Sanitizers on every test run (dev/CI only — never production)

```sh
# debug/test build
-g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer

# data races (separate build; ASan+TSan don't combine)
-fsanitize=thread
```

- ASan catches use-after-free and overflows
  ([memory.md](memory.md)) —
  [AddressSanitizer docs](https://clang.llvm.org/docs/AddressSanitizer.html).
  Leak detection (LeakSanitizer) is on by default only on Linux; on macOS it
  is off — set `ASAN_OPTIONS=detect_leaks=1` where supported and don't count
  a macOS ASan pass as a leak check.
- UBSan catches most of [undefined-behavior.md](undefined-behavior.md) at the
  moment of execution —
  [UBSan docs](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html);
  add `-fno-sanitize-recover=all` in CI so UB fails the build.
- Sanitizers are *bug detectors*, not mitigations: they enlarge the attack
  surface and must not ship
  ([OpenSSF guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html)).
- Where sanitizers can't go (embedded targets), run the suite under
  [Valgrind](https://valgrind.org/docs/manual/quick-start.html) on a host build.

---

## T3. Production hardening flags

Distilled from the
[OpenSSF guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html)
(GCC/Clang, Linux ELF; check the guide for versions and platform variants):

```sh
-O2 -D_FORTIFY_SOURCE=3 \
-fstack-protector-strong -fstack-clash-protection \
-fPIE -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack \
-fno-delete-null-pointer-checks -fno-strict-overflow -fno-strict-aliasing \
-ftrivial-auto-var-init=zero
```

The `-fno-strict-*`/null-check flags trade optimization for making a class of
UB non-exploitable — defense in depth, **not** a license to write the UB
([undefined-behavior.md](undefined-behavior.md) still applies).

---

## T4. Static analysis in CI

- **clang-tidy** with at least `bugprone-*`, `cert-*`, `clang-analyzer-*`
  checks — [clang-tidy docs](https://clang.llvm.org/extra/clang-tidy/); the
  `cert-*` checks enforce CERT rules cited throughout this skill.
- **GCC static analyzer**: `-fanalyzer` —
  [GCC analyzer options](https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html).
- Run both; they find disjoint bugs. Zero-finding policy, same as warnings
  ([Power of Ten, Rule 10](https://spinroot.com/gerard/pdf/P10.pdf)).

---

## T5. Mechanical formatting: clang-format as a CI gate

Keep `.clang-format` at repo root; `clang-format --dry-run --Werror` in CI.
Never review formatting a machine can enforce
([style.md S9](style.md)).

- [clang-format docs](https://clang.llvm.org/docs/ClangFormat.html);
  [style options reference](https://clang.llvm.org/docs/ClangFormatStyleOptions.html).
- A reasonable seed config: [MaJerle/c-code-style](https://github.com/MaJerle/c-code-style)
  ships an MIT-licensed `.clang-format` plus a CI workflow to copy from.

---

## T6. Recommended build matrix

| Build | Flags | Purpose |
|---|---|---|
| dev | T1 warnings + `-g -Og` | daily work |
| test/CI | T1 + T2 sanitizers | correctness gate |
| CI static | T1 + `-fanalyzer`, clang-tidy | analysis gate |
| release | T1 warnings + T3 hardening | what ships |

Compile with **both** GCC and Clang in CI when possible — each catches
diagnostics the other misses, and dual-compiler discipline keeps the code on
the standard instead of one compiler's dialect
([MaJerle](https://github.com/MaJerle/c-code-style) and the
[OpenSSF guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html)
both assume GCC+Clang parity).

---

## T7. Fuzz whatever parses untrusted input

Sanitizers find the bugs your tests reach; fuzzing finds the inputs your tests
never imagined. Any function that parses external bytes (network frames, file
formats, user strings) deserves a fuzz target:

```sh
# libFuzzer: clang-native, links the fuzzer runtime into the target
clang -g -O1 -fsanitize=fuzzer,address,undefined fuzz_parse.c parse.c
```

A target is one function:
`int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)` that calls the
parser. Run fuzzers with ASan+UBSan enabled so crashes are diagnosed at the
moment of corruption. For long-running/coverage-guided campaigns, AFL++ is the
standard alternative.

**Source:**
[libFuzzer documentation](https://llvm.org/docs/LibFuzzer.html);
[AFL++ documentation](https://aflplus.plus/);
[OpenSSF Hardening Guide](https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html)
(sanitizers as test-time instrumentation).

---

## T8. Build-system integration: encode the flag sets once

Make (or any build system) is where T1–T3 become real. The conventions:

- **Respect the standard variable names** — `CFLAGS`, `LDFLAGS`, `CPPFLAGS` —
  and *append* to them (`CFLAGS += ...`), so users and packagers can inject
  their own flags.
- **One place per flag set:** define `WARNINGS`, `SANITIZE`, `HARDEN`
  variables mirroring T1/T2/T3 and compose build types from them
  (`debug: CFLAGS += $(WARNINGS) $(SANITIZE)`), rather than repeating flags
  per target.
- **Real dependency tracking:** generate header dependencies with
  `-MMD -MP` and `-include $(DEPS)` — hand-maintained dependency lists rot.
- **Emit `compile_commands.json`** (CMake: `CMAKE_EXPORT_COMPILE_COMMANDS=ON`;
  Make: via [Bear](https://github.com/rizsotto/Bear)) so clang-tidy and
  clang-format see the true flags per file (see T4/T5).

**Source:**
[GNU Make manual](https://www.gnu.org/software/make/manual/make.html);
[GNU Coding Standards (Makefile conventions)](https://www.gnu.org/prep/standards/standards.html);
[GCC: options controlling the preprocessor (`-MMD`)](https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html);
[clang-tidy docs](https://clang.llvm.org/extra/clang-tidy/) (compilation
database usage).
