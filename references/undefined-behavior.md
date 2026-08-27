# Undefined Behavior: The Optimizer's Contract

UB is not "it crashes" — it is "the compiler may assume this never happens and
transform your program accordingly." Code with UB can pass every test at `-O0`
and break at `-O2`, on the next compiler, or only for the input that matters.
Detection is mechanical: UBSan on every test run ([tooling.md](tooling.md)).

The authoritative UB catalog is
[N3220 Annex J.2](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
a readable overview is
[cppreference: undefined behavior](https://en.cppreference.com/w/c/language/behavior).
Below are the classes that dominate real code.

---

## U1. Know the vocabulary: undefined vs unspecified vs implementation-defined

**Why:** the three get conflated, and only one of them voids the program.

- **Implementation-defined:** the compiler must pick and document a behavior
  (e.g. `sizeof(int)`, right-shift of negative values). Portable code
  documents its assumptions.
- **Unspecified:** one of several valid behaviors, no documentation required
  (e.g. argument evaluation order — [gotchas.md G14](gotchas.md)).
- **Undefined:** the standard places NO requirements — the entire program's
  behavior is invalid, before and after the UB point.

**Source:**
[N3220 §3.5 (behavior definitions, §3.5.1–3.5.4), §4](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[c-faq §11 (ANSI C)](https://c-faq.com/ansi/index.html);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/).

---

## U2. Shifting by ≥ the type's width, or by a negative count, is UB

**Why:** hardware masks the shift count on some ISAs and not others, so the
compiler assumes an out-of-range shift never happens. (Signed arithmetic
overflow, the sibling rule, is covered in [gotchas.md G10](gotchas.md).)

```c
/* BAD: UB when n >= 32 (uint32_t) — common in bitmask helpers */
uint32_t mask = 1u << n;

/* GOOD: reject or clamp the domain first */
if (n >= 32) { return -EINVAL; }
uint32_t mask = UINT32_C(1) << n;
```

**Runnable:** [`demo_u2()` in examples/undefined_behavior.c](../examples/undefined_behavior.c)

**Source:**
[CERT INT34-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int34-c/);
[CERT INT32-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int32-c/);
[N3220 §6.5.8](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf).

---

## U3. Strict aliasing: don't read one type through another type's pointer

**Why:** the optimizer assumes differently-typed pointers don't alias, and
reorders/caches accesses accordingly; pointer-cast "type punning" silently
reads stale values.

```c
/* BAD: float read through int pointer */
float f = 1.0f;
uint32_t bits = *(uint32_t *)&f;

/* GOOD: memcpy is the sanctioned pun — compiles to the same single move */
uint32_t bits;
memcpy(&bits, &f, sizeof bits);
```

`char`/`unsigned char` pointers may inspect any object's bytes; a `union`
member swap is also valid in C (unlike C++).

**Runnable:** [`demo_u3()` in examples/undefined_behavior.c](../examples/undefined_behavior.c)

**Source:**
[CERT EXP39-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/expressions-exp/exp39-c/);
[N3220 §6.5.1 (effective types)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[cppreference: objects and alignment](https://en.cppreference.com/w/c/language/object).

---

## U4. Misaligned access: casting to a stricter alignment is UB before you even dereference

**Why:** parsing code loves `*(uint32_t *)(buf + off)` — on x86 it usually
"works", on ARM it may fault, and the compiler may vectorize assuming
alignment either way.

```c
/* BAD: buf + off has char alignment */
uint32_t v = *(const uint32_t *)(buf + off);

/* GOOD */
uint32_t v;
memcpy(&v, buf + off, sizeof v);
```

**Runnable:** [`demo_u4()` in examples/undefined_behavior.c](../examples/undefined_behavior.c)

**Source:**
[CERT EXP36-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/expressions-exp/exp36-c/);
[N3220 §6.3.2.3 (pointer conversions)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf).

---

## U5. Pointer arithmetic is only defined inside (and one-past) the object

**Why:** merely *forming* `arr - 1` or `arr + n + 1` is UB — no dereference
needed; bounds-check rewrites that "just compute the end pointer first" can
be miscompiled.

```c
/* BAD: p + len may be formed past one-past-the-end; ptr < start is UB too */
if (p + len > end) { ... }          /* if p+len overflows the object: UB */

/* GOOD: compare lengths, not wandering pointers */
if (len > (size_t)(end - p)) { ... }
```

**Runnable:** [`demo_u5()` in examples/undefined_behavior.c](../examples/undefined_behavior.c)

**Source:**
[CERT ARR30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/arrays-arr/arr30-c/);
[N3220 §6.5.7](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[c-faq §6](https://c-faq.com/aryptr/index.html).

---

## U6. Null dereference and the vanishing null check

**Why:** beyond the crash: if code dereferences `p` and *later* checks
`p != NULL`, the compiler may delete the check — the dereference already
"proved" `p` non-null.

```c
/* BAD: check is dead code after the dereference above it */
int len = strlen(s);
if (s == NULL) { return -EINVAL; }

/* GOOD: validate before first use */
if (s == NULL) { return -EINVAL; }
size_t len = strlen(s);
```

**Runnable:** [`demo_u6()` in examples/undefined_behavior.c](../examples/undefined_behavior.c)

**Source:**
[CERT EXP34-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/expressions-exp/exp34-c/).

---

## U7. Data races: unsynchronized cross-thread access is UB, `volatile` is not a fix

**Why:** two threads touching the same object with at least one write and no
synchronization is undefined — not "eventually consistent". `volatile`
prevents neither tearing nor reordering; it is for memory-mapped I/O.

Use `<threads.h>`/pthreads mutexes or C11 `<stdatomic.h>` atomics for every
shared object.

**Runnable:** [`demo_u7()` in examples/undefined_behavior.c](../examples/undefined_behavior.c)

**Source:**
[N3220 §5.1.2.5 (multi-threaded executions and data races)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[CERT CON43-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/concurrency-con/con43-c/)
(do not allow data races);
[CERT CON32-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/concurrency-con/con32-c/)
(the bit-field special case);
[cppreference: atomics](https://en.cppreference.com/w/c/atomic).

---

## U8. Other high-frequency UB (cross-referenced)

- Unsequenced modifications — [gotchas.md G6](gotchas.md)
- Writing string literals — [gotchas.md G8](gotchas.md)
- Use-after-free / double-free — [memory.md M4](memory.md)
- Reading uninitialized objects — [memory.md M8](memory.md)
- Out-of-bounds indexing — [security.md SEC1](security.md)

And a mindset rule: **never "fix" UB by observing that it works.** A test
passing under one compiler proves nothing about the contract. If UBSan or the
standard says it's undefined, restructure the code.

**Source:**
[N3220 Annex J.2 (complete UB list)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[UBSan documentation](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/).
