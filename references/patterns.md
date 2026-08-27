# C Patterns: Structuring Programs Without a Class System

Sourced idioms for encapsulation, initialization, and data structures. Only
patterns traceable to a vetted source appear here — folklore without a source
stays out, per [SOURCES.md](SOURCES.md).

---

## P1. Opaque types: hide the struct behind an incomplete type

**Why:** consumers that can see members will depend on them; an opaque handle
makes the ABI the function set, freeing you to change the layout.

```c
/* parser.h — the public contract */
typedef struct parser parser;           /* incomplete: members invisible */
parser *parser_new(const char *src);
int     parser_run(parser *p);
void    parser_free(parser *p);

/* parser.c — the only place the layout exists */
struct parser {
    const char *src;
    size_t      pos;
    int         depth;
};
```

**Runnable:** [`demo_p1()` in examples/patterns.c](../examples/patterns.c)

**Source:**
[CERT DCL12-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/declarations-and-initialization-dcl/dcl12-c/)
(abstract data types via opaque types);
[cppreference: incomplete types](https://en.cppreference.com/w/c/language/type).

---

## P2. Designated initializers + `{0}`: construct whole values, not field-by-field

**Why:** field-at-a-time initialization misses fields silently when the struct
grows; designated initializers name what they set and zero the rest.

```c
struct server_cfg {
    const char *host;
    uint16_t    port;
    int         backlog;
    bool        reuse_addr;
};

/* GOOD: everything not named is zero; order-independent; grows safely */
struct server_cfg cfg = {
    .host = "127.0.0.1",
    .port = 8080,
    .reuse_addr = true,
};
```

This also gives C a labeled-arguments idiom: take a config struct instead of
six positional parameters.

**Runnable:** [`demo_p2()` in examples/patterns.c](../examples/patterns.c)

**Source:**
[cppreference: struct initialization](https://en.cppreference.com/w/c/language/struct_initialization);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/)
(named initialization as the default);
[N3220 §6.7.11](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf).

---

## P3. Compound literals for one-shot values

**Why:** temporary structs and arrays can be spelled inline, removing
single-use named variables.

```c
draw_point((struct point){ .x = 10, .y = 20 });

int ok = send_all(sock, (const uint8_t[]){ 0x01, 0x02 }, 2);
```

Mind the lifetime: a compound literal inside a function lives until the end of
the enclosing block — don't store pointers to it.

**Runnable:** [`demo_p3()` in examples/patterns.c](../examples/patterns.c)

**Source:**
[cppreference: compound literals](https://en.cppreference.com/w/c/language/compound_literal);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/);
[N3220 §6.5.3.6](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf).

---

## P4. Paired lifecycle functions: `x_new`/`x_free` (or `x_init`/`x_destroy`)

**Why:** every resource-holding type needs a constructor/destructor pair with
consistent naming, so ownership rules are uniform across the codebase
(see [memory.md M1](memory.md)).

- Heap-allocating pair: `T *t_new(...)` / `void t_free(T *)` — `t_free(NULL)`
  must be a safe no-op, mirroring `free`.
- Caller-storage pair: `int t_init(T *, ...)` / `void t_destroy(T *)` — for
  embedding in other structs or the stack.
- Destructors release members in reverse order of acquisition (mirrors the
  goto-cleanup pattern, [error-handling.md E3](error-handling.md)).

**Runnable:** [`demo_p4()` in examples/patterns.c](../examples/patterns.c)

**Source:**
[CERT MEM31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem31-c/);
[CERT MEM12-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/memory-management-mem/mem12-c/);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/).

---

## P5. Flexible array member: one allocation for header + payload

**Why:** the standard replacement for the UB "struct hack": a trailing
unsized array lets a length-prefixed buffer live in a single allocation.

```c
struct packet {
    uint16_t len;
    uint8_t  data[];        /* flexible array member — must be last */
};

struct packet *packet_new(const uint8_t *payload, uint16_t len) {
    struct packet *pkt = malloc(sizeof *pkt + len);
    if (pkt == NULL) { return NULL; }
    pkt->len = len;         /* len already uint16_t: no unchecked narrowing */
    memcpy(pkt->data, payload, len);
    return pkt;
}
```

**Runnable:** [`demo_p5()` in examples/patterns.c](../examples/patterns.c)

**Source:**
[N3220 §6.7.3.2](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf);
[cppreference: struct (flexible array member)](https://en.cppreference.com/w/c/language/struct);
[c-faq §2.6 (struct hack)](https://c-faq.com/struct/index.html).

---

## P6. Function-pointer tables for polymorphism — when you actually need it

**Why:** an ops-table gives interchangeable implementations behind one
interface (the kernel's `file_operations` model) without a class system.

```c
struct codec_ops {
    int (*encode)(void *ctx, const frame *in, buf *out);
    int (*decode)(void *ctx, const buf *in, frame *out);
    void (*close)(void *ctx);
};
```

Use it for genuine plug-points (drivers, codecs, backends). For a fixed set of
cases known at compile time, a plain `switch` is simpler and
statically-analyzable — don't build vtables to look object-oriented.

**Runnable:** [`demo_p6()` in examples/patterns.c](../examples/patterns.c)

**Source:**
kernel VFS `struct file_operations` —
[kernel docs: VFS](https://www.kernel.org/doc/html/latest/filesystems/vfs.html);
[Power of Ten, Rule 9](https://spinroot.com/gerard/pdf/P10.pdf) (limit
function pointers — the counterweight: they defeat static analysis, so spend
them deliberately).

---

## P7. Intrusive linked lists: the node lives inside the object

**Why:** external node allocation doubles allocations and cache misses; the
intrusive form (kernel `list_head` model) embeds links in the object, and one
object can sit on several lists.

```c
struct list_node { struct list_node *next, *prev; };

struct job {
    int              id;
    struct list_node queue_link;      /* this job's place in the run queue */
};
```

Recover the container with the `container_of`/`offsetof` idiom
(`(struct job *)((char *)n - offsetof(struct job, queue_link))`).

**Runnable:** [`demo_p7()` in examples/patterns.c](../examples/patterns.c)

**Source:**
[kernel docs: list API](https://www.kernel.org/doc/html/latest/core-api/list.html);
[cppreference: offsetof](https://en.cppreference.com/w/c/types/offsetof).

---

## P8. Arena (region) allocation: free a whole phase at once

**Why:** when many small objects share one lifetime (a request, a parse, a
frame), per-object `free` is bookkeeping with no benefit; a bump-pointer arena
allocates cheaply and releases everything in O(1) — eliminating leaks and
use-after-free *within* the phase by construction. This is the disciplined,
general-purpose relative of the static-pool rule for safety-critical code
([memory.md M9](memory.md)).

Keep it honest: objects with escape routes out of the phase don't go in the
arena; alignment must be maintained — both per-allocation (round offsets up to
`_Alignof(max_align_t)`) and on the backing storage itself (declare it
`_Alignas(max_align_t)`).

**Runnable:** [`demo_p8()` in examples/patterns.c](../examples/patterns.c)

**Source:**
[Power of Ten, Rule 3](https://spinroot.com/gerard/pdf/P10.pdf) (bounded,
predictable allocation);
[Modern C (Gustedt)](https://gustedt.gitlabpages.inria.fr/modern-c/)
(allocation strategy and lifetimes);
[N3220 §6.2.4 (storage durations)](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf).

---

## P9. `static inline` in headers instead of function-like macros

**Why:** real functions type-check their arguments, evaluate them exactly
once, and debug/profile under their own name — macros do none of that
([gotchas.md G7](gotchas.md)).

```c
/* GOOD: header-safe, zero-cost, type-checked.
 * Preconditions: a is a nonzero power of two; v + a - 1 must not wrap. */
static inline uint32_t align_up(uint32_t v, uint32_t a) {
    assert(a != 0 && (a & (a - 1)) == 0);
    assert(v <= UINT32_MAX - (a - 1));
    return (v + a - 1) & ~(a - 1);
}
```

Reserve macros for what functions can't do: token pasting, stringization,
compile-time constants for array sizes (pre-C23), conditional compilation.

**Runnable:** [`demo_p9()` in examples/patterns.c](../examples/patterns.c)

**Source:**
[CERT PRE00-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/preprocessor-pre/pre00-c/);
[kernel style ch. 12](https://www.kernel.org/doc/html/latest/process/coding-style.html#macros-enums-and-rtl);
[cppreference: inline](https://en.cppreference.com/w/c/language/inline).
