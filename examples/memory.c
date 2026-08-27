/*
 * memory.c — runnable demonstrations for references/memory.md (M1–M9).
 * License: MIT (see LICENSE.md). Build/run: scripts/validate-examples.sh
 */
#ifdef NDEBUG
#error "These examples verify behavior with assert(); build with assertions enabled."
#endif

#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* M1 + M2: one owner per allocation; size from the object; result checked. */
struct node {
    int value;
    struct node *next;
};
static struct node *node_new(int value) {   /* returns caller-owned node */
    struct node *n = malloc(sizeof *n);     /* sizeof *n, not sizeof(type) */
    if (n == NULL) {
        return NULL;
    }
    n->value = value;
    n->next = NULL;
    return n;
}
static void demo_m1_m2(void) {
    struct node *n = node_new(7);
    assert(n != NULL && n->value == 7);
    free(n);                                /* the one owner frees */
}

/* M3: overflow-check size arithmetic before allocating. */
struct rec { uint64_t a, b; };
static struct rec *recs_new(size_t count) {
    if (count > SIZE_MAX / sizeof(struct rec)) {   /* manual guard */
        return NULL;
    }
    return calloc(count, sizeof(struct rec));      /* calloc also checks */
}
static void demo_m3(void) {
    struct rec *rs = recs_new(4);
    assert(rs != NULL && rs[3].a == 0);            /* calloc zeroes */
    free(rs);
    assert(recs_new(SIZE_MAX / 2) == NULL);        /* absurd count rejected */
}

/* M4: null after free; free(NULL) is a safe no-op. */
static void demo_m4(void) {
    char *buf = malloc(16);
    assert(buf != NULL);
    free(buf);
    buf = NULL;          /* later accidental free(buf) is now harmless */
    free(buf);           /* no-op by definition */
    assert(buf == NULL);
}

/* M5: keep the original pointer; never free a moved one. */
static void demo_m5(void) {
    char *original = malloc(8);
    assert(original != NULL);
    memcpy(original, "abcdefg", 8);
    for (char *p = original; *p != '\0'; p++) {   /* iterate a COPY */
        (void)*p;
    }
    free(original);                               /* free the original */
}

/* M6: realloc into a temporary; the old block survives failure. */
static bool grow(char **buf, size_t new_size) {
    char *tmp = realloc(*buf, new_size);
    if (tmp == NULL) {
        return false;      /* *buf still valid — caller decides */
    }
    *buf = tmp;
    return true;
}
static void demo_m6(void) {
    char *buf = malloc(4);
    assert(buf != NULL);
    memcpy(buf, "hi", 3);
    bool grew = grow(&buf, 64);       /* side-effectful call outside assert */
    assert(grew);
    assert(strcmp(buf, "hi") == 0);   /* contents preserved across growth */
    free(buf);
}

/* M7: never return stack storage — fill a caller-provided buffer. */
static int greeting(char *out, size_t out_len) {
    int n = snprintf(out, out_len, "hello");
    return (n < 0 || (size_t)n >= out_len) ? -1 : 0;
}
static void demo_m7(void) {
    char msg[16];
    assert(greeting(msg, sizeof msg) == 0);
    assert(strcmp(msg, "hello") == 0);
    char tiny[3];
    assert(greeting(tiny, sizeof tiny) == -1);   /* truncation reported */
}

/* M8: initialize everything; = {0} zeroes whole aggregates. */
struct config {
    int flags;
    double timeout;
    char name[16];
};
static void demo_m8(void) {
    struct config cfg = {0};
    assert(cfg.flags == 0 && cfg.timeout == 0.0 && cfg.name[0] == '\0');
}

/* M9: fixed pool sized at init — the safety-critical allocation style. */
#define POOL_SIZE 256
static _Alignas(max_align_t) unsigned char pool[POOL_SIZE];  /* backing must be aligned */
static size_t pool_used;
static void *pool_alloc(size_t size) {
    size_t align = alignof(max_align_t);
    size_t start = (pool_used + align - 1) & ~(align - 1);
    if (size > POOL_SIZE - start) {
        return NULL;                    /* exhaustion is explicit, not UB */
    }
    pool_used = start + size;
    return &pool[start];
}
static void demo_m9(void) {
    pool_used = 0;
    void *a = pool_alloc(100);
    void *b = pool_alloc(100);
    assert(a != NULL && b != NULL && a != b);
    assert(((uintptr_t)b % alignof(max_align_t)) == 0);
    assert(pool_alloc(100) == NULL);    /* pool exhausted: clean failure */
}

int main(void) {
    demo_m1_m2(); demo_m3(); demo_m4(); demo_m5();
    demo_m6(); demo_m7(); demo_m8(); demo_m9();
    puts("memory: all demos OK");
    return 0;
}
