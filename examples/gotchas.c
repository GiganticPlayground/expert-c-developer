/*
 * gotchas.c — runnable demonstrations for references/gotchas.md (G1–G17).
 * Each demo_gNN() proves the GOOD form of its rule with asserts.
 * License: MIT (see LICENSE.md). Build/run: scripts/validate-examples.sh
 */
#ifdef NDEBUG
#error "These examples verify behavior with assert(); build with assertions enabled."
#endif

#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum conn_state { DISCONNECTED, CONNECTED };

/* G1: compare with ==; assignment stays its own statement. */
static void demo_g1(void) {
    enum conn_state state = CONNECTED;
    if (state == CONNECTED) {
        assert(state == CONNECTED);
    }
}

/* G2: parenthesize bitwise tests. */
#define FLAG_READY 0x4u
static void demo_g2(void) {
    unsigned x = FLAG_READY;                  /* flag set, low bit clear */
    assert((x & FLAG_READY) == FLAG_READY);   /* GOOD parse: flag detected */
    assert((x & (FLAG_READY == FLAG_READY)) == 0u); /* BAD parse misses the set flag */
}

/* G3: arrays decay — pass the length; compute counts at the array's scope. */
static void clear_ints(int *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buf[i] = 0;
    }
}
static void demo_g3(void) {
    int values[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    size_t count = sizeof values / sizeof values[0];  /* valid HERE, not in callee */
    assert(count == 8);
    clear_ints(values, count);
    assert(values[0] == 0 && values[7] == 0);
}

/* G4: getchar/fgetc return int — EOF stays distinguishable from byte 0xFF.
 * (Demo uses fgetc on a tmpfile; getchar is fgetc(stdin).) */
static void demo_g4(void) {
    FILE *fp = tmpfile();
    assert(fp != NULL);
    int w1 = fputc(0xFF, fp);  /* a real data byte a signed char would alias with EOF */
    int w2 = fputc('A', fp);
    assert(w1 == 0xFF && w2 == 'A');
    rewind(fp);

    int c;                 /* int, NOT char */
    size_t n = 0;
    bool saw_ff = false;
    while ((c = fgetc(fp)) != EOF) {
        if (c == 0xFF) { saw_ff = true; }
        n++;
    }
    assert(n == 2 && saw_ff);   /* the 0xFF byte did not terminate the loop */
    int rc = fclose(fp);
    assert(rc != EOF);
}

/* G5: keep signed/unsigned domains separate; check before subtracting. */
static bool fits_in_buffer(size_t buf_size, size_t offset, size_t count) {
    return offset <= buf_size && count <= buf_size - offset;
}
static void demo_g5(void) {
    assert(fits_in_buffer(64, 8, 16));
    assert(!fits_in_buffer(64, 65, 1));   /* naive `64 - 65` would underflow huge */
    assert(!fits_in_buffer(64, 8, 60));
}

/* G6: one modification per statement. */
static void demo_g6(void) {
    int a[4] = {0};
    size_t i = 1;
    a[i] = (int)i;      /* sequenced: read i, store */
    i++;                /* then modify */
    assert(a[1] == 1 && i == 2);
}

/* G7: parenthesized macro; better, a static inline function. */
#define SQUARE(x) ((x) * (x))
static inline int square(int x) { return x * x; }
static void demo_g7(void) {
    assert(SQUARE(2 + 1) == 9);   /* unparenthesized form would yield 2 + 2 + 1 = 5 */
    assert(square(5) == 25);
}

/* G8: mutate an array copy, not a string literal. */
static void demo_g8(void) {
    char name[] = "bob";          /* writable copy of the literal */
    name[0] = 'B';
    assert(strcmp(name, "Bob") == 0);
    const char *label = "bob";    /* honest type for read-only use */
    assert(label[0] == 'b');
}

/* G9: snprintf truncates AND terminates; detect the truncation. */
static void demo_g9(void) {
    char dst[8];
    const char *src = "this is far too long";
    int n = snprintf(dst, sizeof dst, "%s", src);
    assert(n > 0 && (size_t)n >= sizeof dst);    /* truncation detected */
    assert(dst[sizeof dst - 1] == '\0');         /* still a valid string */
    assert(strlen(dst) == sizeof dst - 1);
}

/* G10: test against the limit BEFORE the signed operation. */
static bool add_checked(int a, int b, int *out) {
    if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b)) {
        return false;
    }
    *out = a + b;
    return true;
}
static void demo_g10(void) {
    int r = 0;
    assert(add_checked(40, 2, &r) && r == 42);
    assert(!add_checked(INT_MAX, 1, &r));     /* rejected without ever overflowing */
    assert(!add_checked(INT_MIN, -1, &r));
}

/* G11: compare floats against a tolerance. */
static void demo_g11(void) {
    double total = 0.1 + 0.2;
    assert(fabs(total - 0.3) < 1e-9);     /* robust; `total == 0.3` is not */
}

/* G12: every case breaks; default always present. */
enum level { COOL, WARM, HOT };
static int actions_for(enum level lv) {
    int actions = 0;
    switch (lv) {
    case WARM:
        actions = 1;            /* fan only */
        break;
    case HOT:
        actions = 2;            /* fan + log */
        break;
    case COOL:
    default:
        actions = 0;
        break;
    }
    return actions;
}
static void demo_g12(void) {
    assert(actions_for(WARM) == 1);   /* no fallthrough into HOT's actions */
    assert(actions_for(HOT) == 2);
    assert(actions_for(COOL) == 0);
}

/* G13: runtime failures handled; asserts reserved for invariants. */
static int open_config(const char *path, FILE **out) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) {              /* handled — NOT assert(fp != NULL) */
        return -1;
    }
    *out = fp;
    return 0;
}
static void demo_g13(void) {
    FILE *fp = NULL;
    assert(open_config("/nonexistent/definitely-missing", &fp) == -1);
    assert(fp == NULL);            /* invariant of the failure contract: fine to assert */
}

/* G14: force evaluation order with sequenced statements. */
static int stack_vals[8];
static size_t stack_top;
static void push(int v) { stack_vals[stack_top++] = v; }
static int pop(void) { return stack_vals[--stack_top]; }
static void demo_g14(void) {
    stack_top = 0;
    push(10);
    push(3);
    int a = pop();     /* 3 — order is now explicit */
    int b = pop();     /* 10 */
    push(a - b);
    assert(pop() == -7);
}

/* G15: division truncates toward zero; normalize % for indexing. */
#define TABLE_SIZE 7
static size_t table_index(int key) {
    int m = key % TABLE_SIZE;
    return (size_t)((m + TABLE_SIZE) % TABLE_SIZE);
}
static void demo_g15(void) {
    assert(-7 / 2 == -3 && -7 % 2 == -1);   /* guaranteed since C99 */
    assert(table_index(-1) == 6);           /* normalized, never negative */
    assert(table_index(8) == 1);
}

/* G16: small unsigned types promote to SIGNED int before arithmetic. */
static void demo_g16(void) {
    uint8_t u = 0xFF;
    assert(~u == -256);              /* ~ operates on the PROMOTED int, not uint8_t */
    uint16_t a = 60000, b = 60000;
    uint32_t p = (uint32_t)a * b;    /* widen FIRST: unpromoted a*b would be
                                        signed int overflow (UB) where int is 32-bit */
    assert(p == 3600000000u);
}

/* G17: ctype functions require an unsigned char value (or EOF). */
static void demo_g17(void) {
    assert(isdigit((unsigned char)'7'));
    char c = (char)0xE9;             /* negative on signed-char ABIs */
    (void)isalpha((unsigned char)c); /* the cast makes this call defined;
                                        isalpha(c) with negative c is UB */
}

int main(void) {
    demo_g1(); demo_g2(); demo_g3(); demo_g4(); demo_g5();
    demo_g6(); demo_g7(); demo_g8(); demo_g9(); demo_g10();
    demo_g11(); demo_g12(); demo_g13(); demo_g14(); demo_g15();
    demo_g16(); demo_g17();
    puts("gotchas: all demos OK");
    return 0;
}
