# C Gotchas & Pitfalls

Classic mistakes that compile fine and fail at runtime — the review checklist.
Format per entry: **Rule → Why → Bad → Good → Runnable → Source** (full URLs
required). All examples original (MIT). Source vetting: see
[SOURCES.md](SOURCES.md).

---

## G1. Never use `=` where you mean `==` (and don't hide it in a condition)

**Why:** assignment in a condition is legal C, evaluates to the assigned value,
and silently replaces a comparison with a store.

```c
/* BAD: always "connected", and clobbers state */
if (state = CONNECTED) { ... }

/* GOOD */
if (state == CONNECTED) { ... }
```

If you genuinely assign-and-test, parenthesize and compare explicitly:
`if ((c = getchar()) != EOF)`.

**Runnable:** [`demo_g1()` in examples/gotchas.c](../examples/gotchas.c)

**Source:** [c-faq §17 (style)](https://c-faq.com/style/index.html);
[UMD C Style Guide](https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/).
Enable `-Wparentheses` (in `-Wall`,
[GCC warning options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html))
to catch it.

---

## G2. `&`, `|` bind *looser* than `==` — parenthesize bitwise tests

**Why:** `x & FLAG == FLAG` parses as `x & (FLAG == FLAG)` → `x & 1`. C's
precedence here is a historical accident.

```c
/* BAD: tests x & 1, not the flag */
if (x & FLAG == FLAG) { ... }

/* GOOD */
if ((x & FLAG) == FLAG) { ... }
```

**Runnable:** [`demo_g2()` in examples/gotchas.c](../examples/gotchas.c)

**Source:** [c-faq §3 (expressions)](https://c-faq.com/expr/index.html);
[CERT EXP00-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/expressions-exp/exp00-c/).

---

## G3. Arrays decay to pointers — `sizeof` a parameter is the pointer size

**Why:** a `T arr[N]` function parameter is rewritten by the language to `T *`.
`sizeof(arr)` inside the function is `sizeof(T *)` (8 on LP64 — i.e. typical
64-bit Unix platforms), not the array size.

```c
/* BAD: n is always 8/sizeof(int) regardless of caller's array */
void clear(int buf[64]) {
    size_t n = sizeof(buf) / sizeof(buf[0]);   /* pointer size! */
}

/* GOOD: pass the length explicitly */
void clear(int *buf, size_t len);
```

**Runnable:** [`demo_g3()` in examples/gotchas.c](../examples/gotchas.c)

**Source:** [c-faq §6 (arrays and pointers)](https://c-faq.com/aryptr/index.html);
[N3220 §6.7.7.4](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf)
(array parameter adjustment);
[CERT ARR01-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/arrays-arr/arr01-c/).

---

## G4. `getchar()` returns `int`, not `char`

**Why:** `EOF` is a negative `int` outside `char` range. Storing the result in
`char` either makes `EOF` undetectable (unsigned char) or aliases a real byte
(0xFF) with EOF (signed char).

```c
/* BAD: loop may never end, or ends on a valid 0xFF byte */
char c;
while ((c = getchar()) != EOF) { ... }

/* GOOD */
int c;
while ((c = getchar()) != EOF) { putchar(c); }
```

**Runnable:** [`demo_g4()` in examples/gotchas.c](../examples/gotchas.c)
(the demo uses `fgetc` on a tmpfile; `getchar()` is `fgetc(stdin)`)

**Source:** [c-faq §12 (stdio)](https://c-faq.com/stdio/index.html);
[CERT FIO34-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/input-output-fio/fio34-c/).

---

## G5. Don't mix signed and unsigned in comparisons

**Why:** usual arithmetic conversions turn the signed operand unsigned;
`-1 > sizeof(x)` is *true* because `-1` becomes `SIZE_MAX`.

```c
/* BAD: sizeof(buf) - offset underflows to huge if offset > sizeof(buf) */
if (count < sizeof(buf) - offset) { ... }

/* GOOD: keep domains separate; check before subtracting */
if (offset <= sizeof(buf) && (size_t)count <= sizeof(buf) - offset) { ... }
```

**Runnable:** [`demo_g5()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[CERT INT02-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/integers-int/int02-c/)
(conversion rules);
[CERT INT30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int30-c/)
(unsigned wrap);
[N3220 §6.3.1.8](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf).
`-Wsign-compare` / `-Wconversion` catch most cases.

---

## G6. Unsequenced modification is undefined — never modify a variable twice in one expression

**Why:** `i = i++ + 1` and `f(i++, i++)` have no defined order; the compiler
may do anything, and different optimization levels do different things.

```c
/* BAD: undefined behavior, not "implementation-defined" */
a[i] = i++;

/* GOOD: one modification per statement */
a[i] = i;
i++;
```

**Runnable:** [`demo_g6()` in examples/gotchas.c](../examples/gotchas.c)

**Source:** [c-faq §3.1–3.8](https://c-faq.com/expr/index.html);
[N3220 §6.5](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf)
(sequencing);
[CERT EXP30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/expressions-exp/exp30-c/).

---

## G7. Function-like macros: parenthesize everything, evaluate arguments once

**Why:** unparenthesized expansion splices into surrounding expressions;
arguments with side effects get evaluated per mention.

```c
/* BAD */
#define SQUARE(x) x * x
int r = SQUARE(a + 1);      /* a + 1 * a + 1 */
int s = SQUARE(v++);        /* v++ * v++ — also UB per G6 */

/* GOOD */
#define SQUARE(x) ((x) * (x))     /* still evaluates x twice — document it */
static inline int square(int x) { return x * x; }   /* better: a function */
```

Multi-statement macros must be wrapped in `do { ... } while (0)` so they behave
as one statement after `if`.

**Runnable:** [`demo_g7()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[CERT PRE01-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/preprocessor-pre/pre01-c/);
[CERT PRE31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/preprocessor-pre/pre31-c/);
[CERT PRE00-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/preprocessor-pre/pre00-c/)
(prefer inline functions);
[kernel style ch. 12](https://www.kernel.org/doc/html/latest/process/coding-style.html#macros-enums-and-rtl);
[c-faq §10 (preprocessor)](https://c-faq.com/cpp/index.html).

---

## G8. String literals are read-only — writing to one is UB

**Why:** `char *s = "text"` points at possibly write-protected storage; the
type system doesn't stop you from writing, the OS or the optimizer will.

```c
/* BAD: may segfault, may silently corrupt, may "work" */
char *name = "bob";
name[0] = 'B';

/* GOOD: array copy if you need to mutate; const pointer if you don't */
char name[] = "bob";        /* mutable copy */
const char *label = "bob";  /* the honest type for a literal */
```

**Runnable:** [`demo_g8()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[N3220 §6.4.5](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[c-faq §1 (declarations)](https://c-faq.com/decl/index.html);
[CERT STR30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/characters-and-strings-str/str30-c/).

---

## G9. `strncpy` does not guarantee null termination

**Why:** if the source fills the buffer, `strncpy` writes no terminator; the
"safe" function produces an unterminated string that overreads later.

```c
/* BAD: dst may not be a string afterward */
strncpy(dst, src, sizeof(dst));

/* GOOD: snprintf truncates AND terminates */
int n = snprintf(dst, sizeof(dst), "%s", src);
if (n < 0 || (size_t)n >= sizeof(dst)) { /* handle truncation */ }
```

**Runnable:** [`demo_g9()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[CERT STR31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/characters-and-strings-str/str31-c/);
[CERT STR32-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/characters-and-strings-str/str32-c/);
[cppreference: strncpy](https://en.cppreference.com/w/c/string/byte/strncpy).

---

## G10. Signed integer overflow is undefined, not wraparound

**Why:** the optimizer assumes it can't happen and deletes your overflow check.
`if (x + 1 < x)` on signed `x` is optimized to `if (0)`.

```c
/* BAD: check itself invokes UB; compilers remove it */
int next = x + 1;
if (next < x) { return ERR_OVERFLOW; }

/* GOOD: test before the operation, against the limit */
if (x > INT_MAX - 1) { return ERR_OVERFLOW; }
int next = x + 1;
```

C23 adds `ckd_add`/`ckd_sub`/`ckd_mul`
([cppreference: `<stdckdint.h>`](https://en.cppreference.com/w/c/numeric#Checked_integer_arithmetic))
— use them when available.

**Runnable:** [`demo_g10()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[CERT INT32-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int32-c/);
[N3220 §6.5, Annex J](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/) on
optimizer contracts.

---

## G11. Don't compare floats for exact equality

**Why:** binary floating point can't represent most decimals; accumulated
rounding makes `==` fail unpredictably.

```c
/* BAD */
if (total == 0.3) { ... }

/* GOOD: compare against a tolerance appropriate to the computation */
if (fabs(total - expected) < 1e-9) { ... }
```

**Runnable:** [`demo_g11()` in examples/gotchas.c](../examples/gotchas.c)

**Source:** [c-faq §14 (floating point)](https://c-faq.com/fp/index.html);
[CERT FLP02-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/floating-point-flp/flp02-c/).

---

## G12. `switch` cases fall through — make every ending explicit

**Why:** a missing `break` silently runs the next case; it's among the most
common C review findings.

```c
/* BAD: WARM also logs "hot" */
switch (level) {
case WARM: start_fan();
case HOT:  log_msg("hot"); break;
}

/* GOOD: break every case; always provide default */
switch (level) {
case WARM:
    start_fan();
    break;
case HOT:
    log_msg("hot");
    break;
default:
    break;
}
```

Use `[[fallthrough]];` (C23) or a `/* fall through */` comment for the rare
intentional case; `-Wimplicit-fallthrough` enforces it.

**Runnable:** [`demo_g12()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[CERT MSC17-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/miscellaneous-msc/msc17-c/);
[BARR-C:2018](https://barrgroup.com/embedded-c-coding-standard) (switch rules);
[UMD C Style Guide](https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/).

---

## G13. `assert` is for invariants, never for side effects or runtime errors

**Why:** `NDEBUG` compiles asserts out entirely — any side effect inside
disappears in release builds; and user/input errors must be handled, not
asserted.

```c
/* BAD: release build never opens the file */
assert((fp = fopen(path, "r")) != NULL);

/* GOOD: handle runtime failure; assert only what cannot happen */
fp = fopen(path, "r");
if (fp == NULL) { return report_error(errno); }
assert(idx < table_len);    /* internal invariant, fine */
```

**Runnable:** [`demo_g13()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[CERT MSC11-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/miscellaneous-msc/msc11-c/);
[Power of Ten, Rule 5](https://spinroot.com/gerard/pdf/P10.pdf) (asserts must
be side-effect free);
[UMD C Style Guide](https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/).

---

## G14. Evaluation order of function arguments is unspecified

**Why:** `f(g(), h())` may call `h` first; code that depends on left-to-right
order breaks across compilers.

```c
/* BAD: which pop happens first? */
push(pop(stack) - pop(stack));

/* GOOD: force the order with sequenced statements */
int a = pop(stack);
int b = pop(stack);
push(a - b);
```

**Runnable:** [`demo_g14()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[N3220 §6.5.3.3](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[c-faq §3.7](https://c-faq.com/expr/index.html);
[CERT EXP10-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/expressions-exp/exp10-c/).

---

## G15. Integer division truncates toward zero; `%` takes the dividend's sign

**Why:** `-7 / 2 == -3` and `-7 % 2 == -1` (since C99). Hash/index code using
`%` on possibly-negative values produces negative indices.

```c
/* BAD: idx can be negative → out-of-bounds read */
int idx = key % TABLE_SIZE;

/* GOOD — two safe options with DIFFERENT semantics: */
size_t idx = (size_t)key % TABLE_SIZE;   /* in-range, deterministic — but NOT
                                            mathematical mod for negative keys
                                            unless TABLE_SIZE is a power of two */
int idx2 = ((key % TABLE_SIZE) + TABLE_SIZE) % TABLE_SIZE;   /* true mod */
```

Both stay in range (safe for hashing); use the normalized form when the
*value* of the index matters, not just its validity.

**Runnable:** [`demo_g15()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[N3220 §6.5.6](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[CERT INT10-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/integers-int/int10-c/).

---

## G16. Small unsigned types promote to *signed* `int` before arithmetic

**Why:** fixed-width types don't make arithmetic safe: any type narrower than
`int` (`uint8_t`, `uint16_t`, plain `char`, `short`) is promoted to signed
`int` before `*`, `<<`, `~`, `-` apply. `u16a * u16b` can be **signed
overflow (UB)** on 32-bit-int platforms, `u8 << 24` can shift into the sign
bit, and `~u8` is a negative `int`.

```c
/* BAD: 60000 * 60000 = 3.6e9 overflows the promoted signed int — UB */
uint16_t a = 60000, b = 60000;
uint32_t p = a * b;

/* GOOD: widen to the unsigned result type BEFORE the operation */
uint32_t p = (uint32_t)a * b;
```

**Runnable:** [`demo_g16()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[CERT INT02-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/integers-int/int02-c/)
(conversion/promotion rules);
[N3220 §6.3.1.1 (integer promotions)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[cppreference: implicit conversions](https://en.cppreference.com/w/c/language/conversion).

---

## G17. `<ctype.h>` functions require an `unsigned char` value (or `EOF`)

**Why:** `isalpha(c)` where `c` is a plain `char` holding a negative value
(any byte ≥ 0x80 on signed-`char` ABIs) is undefined behavior — the argument
must be representable as `unsigned char` or be `EOF`.

```c
/* BAD: UB for bytes >= 0x80 when char is signed */
char c = input[i];
if (isalpha(c)) { ... }

/* GOOD: cast through unsigned char at every ctype call site */
if (isalpha((unsigned char)c)) { ... }
```

**Runnable:** [`demo_g17()` in examples/gotchas.c](../examples/gotchas.c)

**Source:**
[CERT STR37-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/characters-and-strings-str/str37-c/);
[cppreference: isalpha (defined behavior note)](https://en.cppreference.com/w/c/string/byte/isalpha).
