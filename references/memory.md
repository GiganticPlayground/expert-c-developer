# Memory: Allocation, Ownership, Lifetimes

The discipline that prevents the top C defect classes (use-after-free, leaks,
double-free, overflow-sized allocations). Verify all of it mechanically:
AddressSanitizer in tests, always (see [tooling.md](tooling.md)).

---

## M1. Every allocation has exactly one owner

**Why:** leaks and double-frees are both ownership confusion. If you can't
name the function responsible for freeing a pointer, the design is wrong.

- Document transfer in the contract comment: "returns caller-owned buffer" /
  "borrows, does not free".
- A function that stores a pointer it didn't allocate must document whose it is.

**Runnable:** [`demo_m1_m2()` in examples/memory.c](../examples/memory.c)

**Source:**
[CERT MEM31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem31-c/)
(free when no longer needed);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/) (malloc
and lifetime discipline);
[c-faq §7 (memory allocation)](https://c-faq.com/malloc/index.html).

---

## M2. `p = malloc(sizeof *p)` — size from the object, and check the result

**Why:** `sizeof(type)` silently desynchronizes when the pointer's type
changes; an unchecked `malloc` turns exhaustion into a crash at a distant
dereference.

```c
/* BAD: wrong size if `node` is later renamed/re-typed; unchecked */
struct node *n = malloc(sizeof(struct node_v2));   /* stale type after a rename */

/* GOOD */
struct node *n = malloc(sizeof *n);
if (n == NULL) {
    return -ENOMEM;
}
```

**Runnable:** [`demo_m1_m2()` in examples/memory.c](../examples/memory.c)

**Source:**
[CERT MEM35-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem35-c/)
(allocate sufficient memory);
[CERT ERR33-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/error-handling-err/err33-c/)
(handle stdlib errors);
[c-faq §7](https://c-faq.com/malloc/index.html);
[kernel style ch. 14 (allocating memory)](https://www.kernel.org/doc/html/latest/process/coding-style.html#allocating-memory)
(sizeof-of-deref idiom).

---

## M3. Overflow-check size arithmetic before allocating

**Why:** `malloc(count * size)` with attacker-influenced `count` wraps around
and returns a too-small buffer — a classic heap-overflow entry point.

```c
/* BAD: count * sizeof(struct rec) can wrap */
struct rec *arr = malloc(count * sizeof(struct rec));

/* GOOD: calloc checks the multiplication for you */
struct rec *arr = calloc(count, sizeof *arr);

/* GOOD (manual, when calloc doesn't fit): */
if (count > SIZE_MAX / sizeof *arr) { return -EOVERFLOW; }
```

**Runnable:** [`demo_m3()` in examples/memory.c](../examples/memory.c)

**Source:**
[CERT MEM07-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/memory-management-mem/mem07-c/)
(calloc argument bounds);
[CERT INT30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int30-c/);
[cppreference: calloc](https://en.cppreference.com/w/c/memory/calloc).

---

## M4. Never touch freed memory; null the pointer after `free`

**Why:** use-after-free is both a correctness bug and the most exploited C
vulnerability class; a nulled pointer turns silent corruption into a clean
crash and makes double-free harmless (`free(NULL)` is a no-op).

```c
/* BAD */
free(session);
log_msg("closing %s", session->name);   /* use after free */
free(session);                          /* double free */

/* GOOD */
log_msg("closing %s", session->name);
free(session);
session = NULL;
```

**Runnable:** [`demo_m4()` in examples/memory.c](../examples/memory.c)

**Source:**
[CERT MEM30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem30-c/)
(no access to freed memory);
[CERT MEM01-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/memory-management-mem/mem01-c/)
(store new value after free).

---

## M5. Free only what an allocator returned — and with the matching allocator

**Why:** freeing stack objects, string literals, or mid-buffer pointers
corrupts the heap.

```c
/* BAD: not heap pointers / not the original pointer */
char buf[64];
free(buf);
char *p = malloc(100);
p++;
free(p);
```

Keep the original pointer when you iterate; in mixed-allocator codebases
(custom pools, mmap), match release to source.

**Runnable:** [`demo_m5()` in examples/memory.c](../examples/memory.c)

**Source:**
[CERT MEM34-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem34-c/).

---

## M6. `realloc`: never assign to the only copy of the pointer

**Why:** on failure `realloc` returns `NULL` and leaves the old block live —
assigning the result to the same variable leaks the original.

```c
/* BAD: leak on failure */
buf = realloc(buf, new_size);

/* GOOD */
char *tmp = realloc(buf, new_size);
if (tmp == NULL) {
    free(buf);
    return -ENOMEM;
}
buf = tmp;
```

**Runnable:** [`demo_m6()` in examples/memory.c](../examples/memory.c)

**Source:**
[cppreference: realloc](https://en.cppreference.com/w/c/memory/realloc);
[CERT MEM31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem31-c/).

---

## M7. Never return a pointer to automatic (stack) storage

**Why:** the object's lifetime ends at return; the pointer dangles into
reused stack.

```c
/* BAD */
const char *greeting(void) {
    char buf[32];
    snprintf(buf, sizeof buf, "hello");
    return buf;                 /* dangling */
}

/* GOOD: caller-provided buffer (preferred) or heap with documented ownership */
int greeting(char *out, size_t out_len) {
    int n = snprintf(out, out_len, "hello");
    return (n < 0 || (size_t)n >= out_len) ? -1 : 0;
}
```

**Runnable:** [`demo_m7()` in examples/memory.c](../examples/memory.c)

**Source:**
[CERT DCL30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/declarations-and-initialization-dcl/dcl30-c/)
(appropriate storage durations);
[c-faq §7](https://c-faq.com/malloc/index.html);
[cppreference: lifetime](https://en.cppreference.com/w/c/language/lifetime).

---

## M8. Never read uninitialized memory

**Why:** it's UB, it leaks stale data (information disclosure), and it makes
behavior depend on stack garbage.

```c
/* BAD: flags read before ever written on some paths */
int flags;
if (mode == FAST) { flags = O_NONBLOCK; }
open_with(flags);

/* GOOD: initialize at declaration; = {0} zeroes whole aggregates */
int flags = 0;
struct config cfg = {0};
```

**Runnable:** [`demo_m8()` in examples/memory.c](../examples/memory.c)

**Source:**
[CERT EXP33-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/expressions-exp/exp33-c/);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/)
(initialize everything);
[BARR-C:2018](https://barrgroup.com/embedded-c-coding-standard).

---

## M9. Constrained/safety-critical targets: allocate at startup only

**Why:** post-initialization `malloc` introduces unpredictable latency,
fragmentation, and untestable exhaustion paths — which is why flight-software
rules ban it.

For firmware/safety-critical code: fixed pools and static buffers sized at
init; no `malloc`/`free` in the steady state. For general-purpose code this is
a design option (see arenas in [patterns.md](patterns.md)), not a rule.

**Runnable:** [`demo_m9()` in examples/memory.c](../examples/memory.c)

**Source:**
[Power of Ten, Rule 3](https://spinroot.com/gerard/pdf/P10.pdf);
[BARR-C:2018](https://barrgroup.com/embedded-c-coding-standard).
