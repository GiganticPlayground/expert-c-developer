# C Style: Naming, Layout, Organization

The style baseline for new code. **Existing codebases win** — match the
project's established conventions before applying these
([Indian Hill](https://www.doc.ic.ac.uk/lab/cplus/cstyle.html);
[MaJerle](https://github.com/MaJerle/c-code-style)). Enforce mechanically with
clang-format where possible (see [tooling.md](tooling.md)) — don't hand-argue
what a tool can settle.

---

## S1. Source file organization: fixed section order

**Why:** a predictable layout makes any file navigable in seconds.

Order within a `.c` file: file comment → `#include`s (own header first, then
system, then project) → `#define`s/types → file-scope (`static`) data →
`static` function prototypes → function definitions. Including your own header
first proves it is self-contained.

**Source:**
[Indian Hill §File Organization](https://www.doc.ic.ac.uk/lab/cplus/cstyle.html);
[UMD C Style Guide](https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/)
(code organization order);
[GNU Coding Standards](https://www.gnu.org/prep/standards/standards.html).

---

## S2. Headers: include guards, self-contained, minimal

**Why:** unguarded or dependent headers create order-sensitive builds that
break at a distance.

```c
/* GOOD: every header */
#ifndef PROJECT_MODULE_H
#define PROJECT_MODULE_H

#include <stddef.h>     /* the header includes what IT needs, nothing more */

typedef struct parser parser;   /* forward-declare instead of including */

#endif /* PROJECT_MODULE_H */
```

Prefer `#ifndef` guards over `#pragma once` for strict portability (guards are
standard; `once` is a common but non-standard extension).

**Source:**
[CERT PRE06-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/preprocessor-pre/pre06-c/)
(include guards);
[Indian Hill §File Organization](https://www.doc.ic.ac.uk/lab/cplus/cstyle.html);
[MaJerle/c-code-style](https://github.com/MaJerle/c-code-style).

---

## S3. Naming: `snake_case` code, `UPPER_CASE` macros, no reserved names

**Why:** consistent casing encodes what a symbol *is*; leading underscores and
`_t`-suffixed POSIX-colliding names invade namespaces reserved for the
implementation.

- Functions, variables, struct/union members: `lower_snake_case`, descriptive.
- Macros and `#define` constants: `UPPER_SNAKE_CASE`.
- Never begin identifiers with `_` (reserved), and avoid `str`/`mem`/`is` +
  lowercase prefixes (reserved for future libc).

```c
/* BAD: reserved identifier + shouting variable */
int _count;
#define maxRetries 3

/* GOOD */
static size_t retry_count;
#define MAX_RETRIES 3
```

**Source:**
[CERT DCL37-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/declarations-and-initialization-dcl/dcl37-c/)
(reserved identifiers);
[kernel style ch. 4 (naming)](https://www.kernel.org/doc/html/latest/process/coding-style.html#naming);
[UMD C Style Guide](https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/);
[Indian Hill §Naming Conventions](https://www.doc.ic.ac.uk/lab/cplus/cstyle.html).

---

## S4. Braces on every control body, even one-liners

**Why:** the unbraced form invites the classic "add a second statement, only
the first is guarded" defect (the class made famous by Apple's 2014
`goto fail` TLS bug).

```c
/* BAD: the second line always runs */
if (err)
    log_error(err);
    cleanup();

/* GOOD */
if (err) {
    log_error(err);
}
cleanup();
```

**Source:**
[BARR-C:2018](https://barrgroup.com/embedded-c-coding-standard) (braces
required);
[UMD C Style Guide](https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/);
[MaJerle/c-code-style](https://github.com/MaJerle/c-code-style).
(Kernel style allows unbraced single statements — that is a documented
kernel-local exception, not our default:
[kernel style ch. 3](https://www.kernel.org/doc/html/latest/process/coding-style.html#placing-braces-and-spaces).)

---

## S5. Functions: short, one job, few parameters

**Why:** long multi-purpose functions defeat review and testing; safety
standards cap function length for exactly this reason.

Target: a function fits on one screen (~60 lines), does one thing, takes few
parameters; split when you need paragraph comments *inside* a body. Mark
internal functions `static`.

**Source:**
[Power of Ten, Rule 4](https://spinroot.com/gerard/pdf/P10.pdf) (~60-line
functions);
[kernel style ch. 6 (functions)](https://www.kernel.org/doc/html/latest/process/coding-style.html#functions);
[UMD C Style Guide](https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/).

---

## S6. Comments say *why*, headers document contracts

**Why:** a comment restating the code rots instantly; the durable information
is intent, invariants, and units.

- Every public function's header declaration carries a contract comment:
  what it does, ownership of pointers, error returns.
- Inside bodies, comment surprising decisions, not mechanics.
- Delete stale comments in code you touch — a wrong comment is worse than none.

**Source:**
[Indian Hill §Comments](https://www.doc.ic.ac.uk/lab/cplus/cstyle.html);
[kernel style ch. 8 (commenting)](https://www.kernel.org/doc/html/latest/process/coding-style.html#commenting);
[UMD C Style Guide](https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/).

---

## S7. Minimize scope; `const` by default

**Why:** the smaller a name's scope and mutability, the less code you must
read to reason about it.

- Declare variables at first use, initialized (C99+), not in a block at top.
- File-scope objects and internal functions: `static`.
- Pointers to data a function only reads: `const T *`.
- Avoid globals; when unavoidable, one owning module + accessor functions.

```c
/* GOOD */
size_t count_matches(const char *text, char target) {
    size_t n = 0;
    for (const char *p = text; *p != '\0'; p++) {
        if (*p == target) { n++; }
    }
    return n;
}
```

**Source:**
[CERT DCL19-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/declarations-and-initialization-dcl/dcl19-c/)
(minimize scope);
[CERT DCL00-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/declarations-and-initialization-dcl/dcl00-c/)
(const-qualify immutable objects);
[Power of Ten, Rule 6](https://spinroot.com/gerard/pdf/P10.pdf) (smallest
scope);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/).

---

## S8. Use `<stdint.h>`/`<stdbool.h>` types; typedef structs sparingly

**Why:** `int`/`long` widths vary by platform; fixed-width types make range
assumptions explicit. Hiding whether something is a struct or a pointer behind
a typedef obscures cost and semantics.

- Sizes/counts: `size_t`. Fixed ranges: `uint32_t`, `int64_t`, etc. Booleans:
  `bool` (we follow C11+ practice here; MaJerle's anti-`bool` stance is
  overruled by Modern C and the standard library's own direction).
- Typedef opaque handles (see [patterns.md](patterns.md)); otherwise write
  `struct point` openly. Never typedef pointers into "handle-looking" names.

**Source:**
[BARR-C:2018](https://barrgroup.com/embedded-c-coding-standard) (fixed-width
integer rule);
[kernel style ch. 5 (typedefs)](https://www.kernel.org/doc/html/latest/process/coding-style.html#typedefs);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/);
[cppreference: fixed-width integers](https://en.cppreference.com/w/c/types/integer).

---

## S9. Formatting is a tool's job: pick one config, enforce in CI

**Why:** hand-maintained formatting drifts and wastes review time; every
mainstream layout question is a clang-format setting.

Keep a `.clang-format` at repo root; run the formatter as a CI gate. Line
length: pick 80 or 100 and stop discussing it. Spaces vs tabs, brace layout,
pointer-`*` placement — encode once, never argue.

**Source:**
[clang-format](https://clang.llvm.org/docs/ClangFormat.html) and
[style options](https://clang.llvm.org/docs/ClangFormatStyleOptions.html);
[MaJerle/c-code-style](https://github.com/MaJerle/c-code-style) (ships
`.clang-format` + CI check — a good seed config, MIT).
