/*
 * undefined_behavior.c — runnable demonstrations for
 * references/undefined-behavior.md (U2–U7): the DEFINED alternative for each
 * UB class. (U1/U8 are conceptual.) The BAD forms never appear here — they
 * are undefined and must not run.
 * License: MIT (see LICENSE.md). Build/run: scripts/validate-examples.sh
 */
#define _POSIX_C_SOURCE 200809L

#ifdef NDEBUG
#error "These examples verify behavior with assert(); build with assertions enabled."
#endif

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* U2: shifts stay inside the type's width — reject the rest. */
static int shift_mask(unsigned n, uint32_t *out) {
    if (n >= 32) {
        return -EINVAL;             /* 1u << 32 would be UB, not zero */
    }
    *out = UINT32_C(1) << n;
    return 0;
}
static void demo_u2(void) {
    uint32_t mask = 0;
    assert(shift_mask(4, &mask) == 0 && mask == 0x10u);
    assert(shift_mask(31, &mask) == 0 && mask == 0x80000000u);
    assert(shift_mask(32, &mask) == -EINVAL);
}

/* U3: memcpy is the sanctioned type pun — no aliasing violation. */
static void demo_u3(void) {
    float f = 1.0f;
    uint32_t bits;
    memcpy(&bits, &f, sizeof bits);          /* not *(uint32_t *)&f */
    float g;
    memcpy(&g, &bits, sizeof g);
    assert(g == f);   /* round-trip is representation-agnostic; exact == is
                         correct here because g is a bit-for-bit copy */
#if defined(__STDC_IEC_559__) || defined(__STDC_IEC_60559_BFP__)
    assert(bits == UINT32_C(0x3F800000));    /* IEEE-754 single for 1.0 —
                                                only guaranteed under IEC 60559 */
#endif
}

/* U4: read unaligned data with memcpy, never a stricter-aligned cast. */
static void demo_u4(void) {
    unsigned char wire[7] = {0xAA, 0x78, 0x56, 0x34, 0x12, 0xBB, 0xCC};
    uint32_t v;
    memcpy(&v, wire + 1, sizeof v);          /* wire+1 has char alignment */
    unsigned char le[4] = {0x78, 0x56, 0x34, 0x12};
    uint32_t expect;
    memcpy(&expect, le, sizeof expect);
    assert(v == expect);
}

/* U5: compare lengths, not wandering pointers. */
static bool range_fits(const char *p, const char *end, size_t len) {
    return len <= (size_t)(end - p);         /* p..end is a valid object range */
}
static void demo_u5(void) {
    char buf[16];
    const char *end = buf + sizeof buf;      /* one-past-the-end: allowed */
    assert(range_fits(buf, end, 16));
    assert(!range_fits(buf + 10, end, 7));   /* no `p + 7 > end` pointer formed */
}

/* U6: validate before FIRST use — never dereference then check. */
static int name_len(const char *s, size_t *out) {
    if (s == NULL) {
        return -EINVAL;             /* the check comes first, so it survives */
    }
    *out = strlen(s);
    return 0;
}
static void demo_u6(void) {
    size_t len = 0;
    assert(name_len("carol", &len) == 0 && len == 5);
    assert(name_len(NULL, &len) == -EINVAL);
}

/* U7: shared state under a mutex — never bare, never just volatile. */
#define U7_THREADS 4
#define U7_INCREMENTS 10000
static struct {
    pthread_mutex_t lock;
    long counter;
} shared = { PTHREAD_MUTEX_INITIALIZER, 0 };

static void *incrementer(void *arg) {
    (void)arg;
    for (int i = 0; i < U7_INCREMENTS; i++) {
        pthread_mutex_lock(&shared.lock);
        shared.counter++;                    /* every access synchronized */
        pthread_mutex_unlock(&shared.lock);
    }
    return NULL;
}
static void demo_u7(void) {
    pthread_t threads[U7_THREADS];
    for (size_t i = 0; i < U7_THREADS; i++) {
        int rc = pthread_create(&threads[i], NULL, incrementer, NULL);
        assert(rc == 0);          /* side-effectful call outside assert (G13) */
    }
    for (size_t i = 0; i < U7_THREADS; i++) {
        int rc = pthread_join(threads[i], NULL);
        assert(rc == 0);
    }
    assert(shared.counter == (long)U7_THREADS * U7_INCREMENTS);  /* no lost updates */
}

int main(void) {
    demo_u2(); demo_u3(); demo_u4(); demo_u5(); demo_u6(); demo_u7();
    puts("undefined_behavior: all demos OK");
    return 0;
}
