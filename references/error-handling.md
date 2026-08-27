# Error Handling in C

C has no exceptions: every failure travels through return values, and every
ignored return value is a latent bug. The pattern language here is small —
apply it uniformly.

---

## E1. Check every fallible call — no naked stdlib/syscall returns

**Why:** `fopen`, `malloc`, `snprintf`, `write`, `fclose` — all report
failure via their return value; ignoring it converts a recoverable error into
corruption later.

```c
/* BAD */
fclose(fp);                        /* write errors surface here — discarded */

/* GOOD */
if (fclose(fp) == EOF) {
    log_errno("close failed", errno);
    return -1;
}
```

Even "can't fail here" calls get `(void)` casts only with a comment saying why.

**Runnable:** [`demo_e1()` in examples/error_handling.c](../examples/error_handling.c)

**Source:**
[CERT ERR33-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/error-handling-err/err33-c/);
[CERT FIO42-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/input-output-fio/fio42-c/)
(close files);
[Power of Ten, Rule 7](https://spinroot.com/gerard/pdf/P10.pdf) (check return
values of all non-void functions).

---

## E2. `errno` discipline: check it only after a call signals failure

**Why:** successful calls may still scribble on `errno`; reading it without a
failing return value reports garbage.

```c
/* BAD: errno may be stale or clobbered by the successful call */
long v = strtol(s, &end, 10);
if (errno == ERANGE) { ... }

/* GOOD: zero before, test the result, then read errno */
errno = 0;
long v = strtol(s, &end, 10);
if (end == s) { return ERR_NOT_A_NUMBER; }
if (errno == ERANGE) { return ERR_OUT_OF_RANGE; }
```

**Runnable:** [`demo_e2()` in examples/error_handling.c](../examples/error_handling.c)

**Source:**
[CERT ERR30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/error-handling-err/err30-c/);
[cppreference: errno](https://en.cppreference.com/w/c/error/errno).

---

## E3. The goto-cleanup pattern is the correct C idiom for multi-resource functions

**Why:** without it, every error path repeats the teardown of everything
acquired so far — and one path always forgets one resource. This is the one
sanctioned use of `goto`: forward, to labeled cleanup, in reverse order of
acquisition.

```c
int process(const char *path) {
    int ret = -1;
    FILE *fp = fopen(path, "r");
    if (fp == NULL) { goto out; }

    char *buf = malloc(BUF_LEN);
    if (buf == NULL) { goto out_close; }

    if (parse(fp, buf, BUF_LEN) != 0) { goto out_free; }
    ret = 0;

out_free:
    free(buf);
out_close:
    fclose(fp);
out:
    return ret;
}
```

**Runnable:** [`demo_e3()` in examples/error_handling.c](../examples/error_handling.c)

**Source:**
[kernel style ch. 7 (centralized exiting)](https://www.kernel.org/doc/html/latest/process/coding-style.html#centralized-exiting-of-functions);
[CERT MEM12-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/memory-management-mem/mem12-c/)
(goto chain for releasing resources).

---

## E4. Validate and bail early; keep the happy path unindented

**Why:** guard clauses at the top make the function's contract visible and
keep the main logic at one indentation level; arrow-shaped nesting hides it.

```c
/* GOOD */
int enqueue(queue *q, const item *it) {
    if (q == NULL || it == NULL) { return -EINVAL; }
    if (q->len == q->cap)        { return -ENOSPC; }

    q->items[q->len++] = *it;
    return 0;
}
```

**Runnable:** [`demo_e4()` in examples/error_handling.c](../examples/error_handling.c)

**Source:**
[kernel style ch. 1 (indentation — deep nesting means broken code)](https://www.kernel.org/doc/html/latest/process/coding-style.html#indentation);
[CERT API00-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/application-programming-interfaces-api/api00-c/)
(validate function arguments).

---

## E5. Pick ONE error convention per codebase and never mix

**Why:** callers must be able to test errors without reading each callee.

Common conventions — any is fine, mixing is not:
- `0` success / negative `errno`-style code (kernel/POSIX flavor);
- `0` success / positive enum error code + out-parameters for results;
- `NULL`/valid-pointer for allocation-like functions, details via `errno`.

Reserve the return value channel for status; return data through out-params
when a function can fail.

**Runnable:** [`demo_e5_e6()` in examples/error_handling.c](../examples/error_handling.c)

**Source:**
[CERT ERR02-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/error-handling-err/err02-c/)
(avoid in-band error indicators — and when the codebase uses them, be
consistent and explicit);
[GNU Coding Standards](https://www.gnu.org/prep/standards/standards.html)
(semantics conventions);
[Indian Hill](https://www.doc.ic.ac.uk/lab/cplus/cstyle.html).

---

## E6. Handle errors once, at the level that has context

**Why:** logging at every level produces noise storms; swallowing at any level
produces silent corruption. The layer that can *decide* (retry, abort,
degrade) reports; layers below translate and propagate.

```c
/* BAD: logs AND returns — caller logs again; or worse, returns 0 anyway */
if (read_block(fd, blk) != 0) {
    perror("read_block");
    return 0;               /* swallowed! */
}

/* GOOD: propagate untouched; the request handler logs once with context */
if (read_block(fd, blk) != 0) { return -EIO; }
```

**Runnable:** [`demo_e5_e6()` in examples/error_handling.c](../examples/error_handling.c)

**Source:**
[CERT ERR00-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/error-handling-err/err00-c/)
(adopt and document a consistent error-handling policy).

---

## E7. Runtime errors are handled; impossibilities are asserted

**Why:** the split keeps release behavior defined: `assert` documents and
checks invariants during development (and vanishes under `NDEBUG`), while
anything reachable from input, environment, or hardware gets a real handled
path. See [gotchas.md G13](gotchas.md) for the side-effect trap.

**Runnable:** [`demo_e7()` in examples/error_handling.c](../examples/error_handling.c)

**Source:**
[CERT MSC11-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/miscellaneous-msc/msc11-c/);
[Power of Ten, Rule 5](https://spinroot.com/gerard/pdf/P10.pdf).

---

## E8. POSIX I/O: partial transfers and `EINTR` are normal — loop, don't assume

**Why:** `read`/`write` may transfer *fewer* bytes than requested (sockets,
pipes, signals) and may fail with `errno == EINTR` without anything being
wrong. A bare `read(fd, buf, n)` that assumes `n` bytes is the most common
real-world POSIX I/O bug.

```c
/* BAD: assumes the whole buffer arrived */
read(fd, buf, len);

/* GOOD: loop until done, retry EINTR, distinguish EOF from error */
size_t got = 0;
while (got < len) {
    ssize_t n = read(fd, (char *)buf + got, len - got);
    if (n < 0) {
        if (errno == EINTR) { continue; }
        return -1;
    }
    if (n == 0) { break; }      /* EOF */
    got += (size_t)n;
}
```

(POSIX-specific; ISO C `fread` handles the looping itself — check `ferror`.)

**Runnable:** [`demo_e8()` in examples/error_handling.c](../examples/error_handling.c)

**Source:**
[POSIX.1-2017: read()](https://pubs.opengroup.org/onlinepubs/9699919799/functions/read.html);
[POSIX.1-2017: write()](https://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html);
[CERT FIO37-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/input-output-fio/fio37-c/)
(do not assume character data was read).

---

## E9. Signal handlers: set a `volatile sig_atomic_t` flag and nothing else

**Why:** a signal can arrive between any two instructions; inside the handler,
almost nothing is safe — `printf`, `malloc`, and locks can deadlock or corrupt
state. Only async-signal-safe functions and lock-free atomic flags are
permitted; the main loop observes the flag and does the real work.

```c
/* BAD: printf/malloc in a handler — undefined/async-unsafe */
static void on_term(int sig) { printf("shutting down\n"); cleanup(); }

/* GOOD: flag only; the main loop reacts */
static volatile sig_atomic_t stop_requested;
static void on_term(int sig) { (void)sig; stop_requested = 1; }
```

**Runnable:** [`demo_e9()` in examples/error_handling.c](../examples/error_handling.c)

**Source:**
[CERT SIG30-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/signals-sig/sig30-c/)
(only async-signal-safe functions in handlers);
[CERT SIG31-C](https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/signals-sig/sig31-c/)
(no shared-object access in handlers);
[cppreference: sig_atomic_t](https://en.cppreference.com/w/c/program/sig_atomic_t).
