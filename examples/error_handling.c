/*
 * error_handling.c — runnable demonstrations for references/error-handling.md
 * (E1–E9). E8/E9 use POSIX APIs (pipe, sigaction); the rest are ISO C.
 * License: MIT (see LICENSE.md). Build/run: scripts/validate-examples.sh
 */
#define _POSIX_C_SOURCE 200809L

#ifdef NDEBUG
#error "These examples verify behavior with assert(); build with assertions enabled."
#endif

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum parse_result { PARSE_OK, PARSE_NOT_A_NUMBER, PARSE_OUT_OF_RANGE };

/* E1: every fallible call checked — including fclose. */
static void demo_e1(void) {
    FILE *fp = tmpfile();
    assert(fp != NULL);
    assert(fputs("data\n", fp) >= 0);
    int rc = fclose(fp);                  /* write errors surface at close */
    assert(rc != EOF);                    /* call outside assert (see G13) */
}

/* E2: errno discipline around strtol. */
static enum parse_result parse_long(const char *s, long *out) {
    char *end = NULL;
    errno = 0;                            /* zero BEFORE the call */
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') {
        return PARSE_NOT_A_NUMBER;        /* result checked FIRST */
    }
    if (errno == ERANGE) {
        return PARSE_OUT_OF_RANGE;        /* errno read only after failure signal */
    }
    *out = v;
    return PARSE_OK;
}
static void demo_e2(void) {
    long v = 0;
    assert(parse_long("123", &v) == PARSE_OK && v == 123);
    assert(parse_long("abc", &v) == PARSE_NOT_A_NUMBER);
    assert(parse_long("999999999999999999999999", &v) == PARSE_OUT_OF_RANGE);
}

/* E3: goto-cleanup — one exit path, teardown in reverse acquisition order. */
#define BUF_LEN 64
static int copy_prefix(const char *path, char *out, size_t out_len) {
    int ret = -1;
    FILE *fp = fopen(path, "r");
    if (fp == NULL) { goto out; }

    char *buf = malloc(BUF_LEN);
    if (buf == NULL) { goto out_close; }

    size_t n = fread(buf, 1, BUF_LEN - 1, fp);
    if (ferror(fp)) { goto out_free; }    /* short fread: EOF is fine, error is not */
    buf[n] = '\0';
    int w = snprintf(out, out_len, "%s", buf);
    if (w < 0 || (size_t)w >= out_len) { goto out_free; }
    ret = 0;

out_free:
    free(buf);
out_close:
    fclose(fp);
out:
    return ret;
}
static void demo_e3(void) {
    char out[BUF_LEN];
    assert(copy_prefix("/nonexistent/missing", out, sizeof out) == -1);
    /* failure path released nothing it didn't acquire; success path: */
    FILE *fp = fopen("example_e3.tmp", "w");
    assert(fp != NULL);
    int w = fputs("hello", fp);           /* side-effectful calls stay outside */
    int rc = fclose(fp);                  /* asserts (see G13) */
    assert(w >= 0 && rc != EOF);
    assert(copy_prefix("example_e3.tmp", out, sizeof out) == 0);
    assert(strcmp(out, "hello") == 0);
    (void)remove("example_e3.tmp");       /* best-effort demo cleanup */
}

/* E4: guard clauses first; happy path unindented. */
#define QUEUE_CAP 2
struct queue {
    int items[QUEUE_CAP];
    size_t len;
};
static int enqueue(struct queue *q, const int *it) {
    if (q == NULL || it == NULL) { return -EINVAL; }
    if (q->len == QUEUE_CAP)     { return -ENOSPC; }

    q->items[q->len++] = *it;
    return 0;
}
static void demo_e4(void) {
    struct queue q = {0};
    int v = 1;
    assert(enqueue(NULL, &v) == -EINVAL);
    assert(enqueue(&q, &v) == 0 && enqueue(&q, &v) == 0);
    assert(enqueue(&q, &v) == -ENOSPC);
}

/* E5 + E6: one convention (0 / negative errno-style), propagate untouched. */
static int read_block(bool ok) {          /* stand-in for a lower layer */
    return ok ? 0 : -EIO;
}
static int load_record(bool ok) {
    int err = read_block(ok);
    if (err != 0) {
        return err;       /* propagate the SAME code; no logging here, no swallowing */
    }
    return 0;
}
static void demo_e5_e6(void) {
    assert(load_record(true) == 0);
    assert(load_record(false) == -EIO);   /* caller sees the original cause */
}

/* E7: runtime errors handled; impossibilities asserted. */
static int table[4] = {10, 20, 30, 40};
static int table_get(size_t idx) {
    assert(idx < 4);       /* internal invariant: callers were validated upstream */
    return table[idx];
}
static void demo_e7(void) {
    FILE *fp = fopen("/nonexistent/missing", "r");
    if (fp == NULL) {
        /* Reachable-from-environment failure: handled, never asserted.
         * (ISO C doesn't guarantee errno is set by fopen — POSIX does.) */
    } else {
        int rc = fclose(fp);
        assert(rc != EOF);
    }
    assert(table_get(2) == 30);           /* invariant checked, not handled */
}

/* E8: read()/write() may transfer fewer bytes than asked — loop, retry EINTR. */
static ssize_t read_all(int fd, void *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = read(fd, (char *)buf + got, len - got);
        if (n < 0) {
            if (errno == EINTR) { continue; }   /* interrupted, not an error */
            return -1;
        }
        if (n == 0) { break; }                  /* EOF */
        got += (size_t)n;
    }
    return (ssize_t)got;
}
static void demo_e8(void) {
    int fds[2];
    int rc = pipe(fds);
    assert(rc == 0);
    const char msg[] = "chunked";
    ssize_t w = write(fds[1], msg, sizeof msg);
    assert(w == (ssize_t)sizeof msg);
    rc = close(fds[1]);
    assert(rc == 0);

    char buf[sizeof msg];
    ssize_t r = read_all(fds[0], buf, sizeof buf);   /* never a bare read() */
    assert(r == (ssize_t)sizeof buf && strcmp(buf, "chunked") == 0);
    rc = close(fds[0]);
    assert(rc == 0);
}

/* E9: a signal handler only sets a volatile sig_atomic_t flag. */
static volatile sig_atomic_t got_signal;
static void on_signal(int sig) {
    (void)sig;
    got_signal = 1;      /* async-signal-safe; no printf/malloc/locks in here */
}
static void demo_e9(void) {
    struct sigaction sa = {0};
    sa.sa_handler = on_signal;
    int rc = sigemptyset(&sa.sa_mask);
    assert(rc == 0);
    rc = sigaction(SIGUSR1, &sa, NULL);
    assert(rc == 0);
    rc = raise(SIGUSR1);
    assert(rc == 0);
    assert(got_signal == 1);   /* the main loop reads the flag and reacts */
}

int main(void) {
    demo_e1(); demo_e2(); demo_e3(); demo_e4(); demo_e5_e6(); demo_e7();
    demo_e8(); demo_e9();
    puts("error_handling: all demos OK");
    return 0;
}
