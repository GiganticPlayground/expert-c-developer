/*
 * patterns.c — runnable demonstrations for references/patterns.md (P1–P9).
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

/* P1: opaque type — the "header" exposes only the handle and functions.
 * (In a real project the typedef lives in parser.h; the struct in parser.c.) */
typedef struct parser parser;
static parser *parser_new(const char *src);
static size_t  parser_count_words(parser *p);
static void    parser_free(parser *p);

struct parser {                    /* private: only this "file" sees members */
    const char *src;
    size_t      pos;
};
static parser *parser_new(const char *src) {
    parser *p = malloc(sizeof *p);
    if (p == NULL) { return NULL; }
    p->src = src;
    p->pos = 0;
    return p;
}
static size_t parser_count_words(parser *p) {
    size_t words = 0;
    bool in_word = false;
    for (const char *s = p->src; *s != '\0'; s++) {
        bool space = (*s == ' ');
        if (!space && !in_word) { words++; }
        in_word = !space;
    }
    return words;
}
static void parser_free(parser *p) { free(p); }   /* free(NULL)-safe */
static void demo_p1(void) {
    parser *p = parser_new("three word phrase");
    assert(p != NULL);
    assert(parser_count_words(p) == 3);
    parser_free(p);
}

/* P2: designated initializers — name what you set, zero the rest. */
struct server_cfg {
    const char *host;
    uint16_t    port;
    int         backlog;
    bool        reuse_addr;
};
static void demo_p2(void) {
    struct server_cfg cfg = {
        .host = "127.0.0.1",
        .port = 8080,
        .reuse_addr = true,
    };
    assert(cfg.backlog == 0);            /* unnamed field: zeroed, not garbage */
    assert(cfg.port == 8080 && cfg.reuse_addr);
}

/* P3: compound literals for one-shot values. */
struct point { int x, y; };
static int manhattan(struct point p) { return abs(p.x) + abs(p.y); }
static int sum_bytes(const uint8_t *b, size_t n) {
    int s = 0;
    for (size_t i = 0; i < n; i++) { s += b[i]; }
    return s;
}
static void demo_p3(void) {
    assert(manhattan((struct point){ .x = 3, .y = -4 }) == 7);
    assert(sum_bytes((const uint8_t[]){ 1, 2, 3 }, 3) == 6);
}

/* P4: paired lifecycle functions; destructor tears down in reverse order. */
struct buffer {
    char  *data;
    size_t cap;
};
static struct buffer *buffer_new(size_t cap) {
    struct buffer *b = malloc(sizeof *b);
    if (b == NULL) { return NULL; }
    b->data = malloc(cap);
    if (b->data == NULL) {
        free(b);                          /* partial construction unwound */
        return NULL;
    }
    b->cap = cap;
    return b;
}
static void buffer_free(struct buffer *b) {
    if (b == NULL) { return; }            /* free(NULL) semantics */
    free(b->data);                        /* members first, then the shell */
    free(b);
}
static void demo_p4(void) {
    struct buffer *b = buffer_new(32);
    assert(b != NULL && b->cap == 32);
    buffer_free(b);
    buffer_free(NULL);                    /* must be a safe no-op */
}

/* P5: flexible array member — header + payload in one allocation. */
struct packet {
    uint16_t len;
    uint8_t  data[];                      /* must be the last member */
};
static struct packet *packet_new(const uint8_t *payload, uint16_t len) {
    struct packet *pkt = malloc(sizeof *pkt + len);
    if (pkt == NULL) { return NULL; }
    pkt->len = len;
    memcpy(pkt->data, payload, len);
    return pkt;
}
static void demo_p5(void) {
    const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    struct packet *pkt = packet_new(payload, sizeof payload);
    assert(pkt != NULL && pkt->len == 4 && pkt->data[3] == 0xEF);
    free(pkt);                            /* ONE free for header + payload */
}

/* P6: ops table — interchangeable implementations behind one interface. */
struct transform_ops {
    char (*apply)(char c);
};
static char to_upper(char c) { return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c; }
static char to_lower(char c) { return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c; }
static void transform(const struct transform_ops *ops, char *s) {
    for (; *s != '\0'; s++) { *s = ops->apply(*s); }
}
static void demo_p6(void) {
    char a[] = "Hello";
    transform(&(struct transform_ops){ .apply = to_upper }, a);
    assert(strcmp(a, "HELLO") == 0);
    transform(&(struct transform_ops){ .apply = to_lower }, a);
    assert(strcmp(a, "hello") == 0);
}

/* P7: intrusive list — links live inside the object; container_of recovers it. */
struct list_node { struct list_node *next; };
#define container_of(ptr, type, member) \
    ((type *)(void *)((char *)(ptr) - offsetof(type, member)))
struct job {
    int              id;
    struct list_node queue_link;
};
static void demo_p7(void) {
    struct job jobs[3] = {
        { .id = 1, .queue_link = { NULL } },
        { .id = 2, .queue_link = { NULL } },
        { .id = 3, .queue_link = { NULL } },
    };
    struct list_node *head = &jobs[0].queue_link;   /* no extra allocations */
    jobs[0].queue_link.next = &jobs[1].queue_link;
    jobs[1].queue_link.next = &jobs[2].queue_link;

    int sum = 0;
    for (struct list_node *n = head; n != NULL; n = n->next) {
        sum += container_of(n, struct job, queue_link)->id;
    }
    assert(sum == 6);
}

/* P8: arena — bump allocation, whole-phase release in O(1). */
struct arena {
    unsigned char *base;
    size_t         cap;
    size_t         used;
};
static void *arena_alloc(struct arena *a, size_t size) {
    size_t align = alignof(max_align_t);
    size_t start = (a->used + align - 1) & ~(align - 1);
    if (start > a->cap || size > a->cap - start) {
        return NULL;
    }
    a->used = start + size;
    return a->base + start;
}
static void arena_reset(struct arena *a) { a->used = 0; }   /* frees EVERYTHING */
static void demo_p8(void) {
    _Alignas(max_align_t) unsigned char backing[512];   /* backing must be aligned */
    struct arena a = { .base = backing, .cap = sizeof backing, .used = 0 };
    char *s1 = arena_alloc(&a, 100);
    char *s2 = arena_alloc(&a, 100);
    assert(s1 != NULL && s2 != NULL && s2 != s1);
    assert(arena_alloc(&a, 500) == NULL);   /* exhaustion is explicit */
    arena_reset(&a);                        /* whole phase gone, no per-object frees */
    assert(arena_alloc(&a, 500) != NULL);
}

/* P9: static inline instead of a function-like macro.
 * Preconditions (asserted): a is a nonzero power of two; v + a - 1 must not wrap. */
static inline uint32_t align_up(uint32_t v, uint32_t a) {
    assert(a != 0 && (a & (a - 1)) == 0);
    assert(v <= UINT32_MAX - (a - 1));
    return (v + a - 1) & ~(a - 1);
}
static void demo_p9(void) {
    assert(align_up(13, 8) == 16);
    assert(align_up(16, 8) == 16);
    uint32_t v = 13;
    assert(align_up(v++, 8) == 16 && v == 14);   /* argument evaluated ONCE */
}

int main(void) {
    demo_p1(); demo_p2(); demo_p3(); demo_p4(); demo_p5();
    demo_p6(); demo_p7(); demo_p8(); demo_p9();
    puts("patterns: all demos OK");
    return 0;
}
