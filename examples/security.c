/*
 * security.c — runnable demonstrations for references/security.md (SEC1–SEC8).
 * (SEC9, hardening flags, is exercised by scripts/validate-examples.sh itself.)
 * License: MIT (see LICENSE.md). Build/run: scripts/validate-examples.sh
 */
#define _POSIX_C_SOURCE 200809L
#define _DARWIN_C_SOURCE            /* macOS: keep mkdtemp/O_NOFOLLOW visible */

#ifdef NDEBUG
#error "These examples verify behavior with assert(); build with assertions enabled."
#endif

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* SEC1: every write bounded; truncation detected, not ignored. */
static int set_name(char *dst, size_t dst_len, const char *user_input) {
    int n = snprintf(dst, dst_len, "%s", user_input);
    if (n < 0 || (size_t)n >= dst_len) {
        return -ENAMETOOLONG;
    }
    return 0;
}
static void demo_sec1(void) {
    char name[8];
    assert(set_name(name, sizeof name, "alice") == 0);
    assert(strcmp(name, "alice") == 0);
    assert(set_name(name, sizeof name, "much-too-long-for-the-buffer")
           == -ENAMETOOLONG);
}

/* SEC2: fgets, not gets; bounded line input with newline handling. */
static void demo_sec2(void) {
    FILE *fp = tmpfile();
    assert(fp != NULL);
    assert(fputs("first line\nsecond\n", fp) >= 0);
    rewind(fp);

    char line[32];
    char *got = fgets(line, (int)sizeof line, fp);   /* outside assert (G13) */
    assert(got != NULL);
    line[strcspn(line, "\n")] = '\0';         /* strip the newline if present */
    assert(strcmp(line, "first line") == 0);
    int rc = fclose(fp);
    assert(rc != EOF);
}

/* SEC3: constant format string; hostile input rides as DATA. */
static void demo_sec3(void) {
    const char *hostile = "100%n of users %s";  /* would be lethal as a format */
    char out[64];
    int n = snprintf(out, sizeof out, "%s", hostile);   /* literal format */
    assert(n > 0 && strcmp(out, hostile) == 0);         /* %n/%s inert as data */
}

/* SEC4: integer checks BEFORE the operation. */
static bool mul_size_ok(size_t a, size_t b) {
    return b == 0 || a <= SIZE_MAX / b;
}
static bool narrow_to_u16(size_t v, uint16_t *out) {
    if (v > UINT16_MAX) {
        return false;                     /* range-check before the cast */
    }
    *out = (uint16_t)v;
    return true;
}
static void demo_sec4(void) {
    assert(mul_size_ok(1000, 1000));
    assert(!mul_size_ok(SIZE_MAX / 2, 3));
    uint16_t port = 0;
    assert(narrow_to_u16(8080, &port) && port == 8080);
    assert(!narrow_to_u16(70000, &port));
}

/* SEC5: allowlist validation instead of shelling out. */
static bool filename_is_safe(const char *name) {
    if (name == NULL || name[0] == '\0' || name[0] == '-') {
        return false;
    }
    for (const char *p = name; *p != '\0'; p++) {
        unsigned char c = (unsigned char)*p;
        if (!isalnum(c) && c != '.' && c != '_' && c != '-') {
            return false;
        }
    }
    return strstr(name, "..") == NULL;
}
static void demo_sec5(void) {
    assert(filename_is_safe("report_2026-08.txt"));
    assert(!filename_is_safe("x; rm -rf ~"));     /* injection attempt rejected */
    assert(!filename_is_safe("../etc/passwd"));
    assert(!filename_is_safe("--output=evil"));
}

/* SEC6: atomic create — no check-then-use gap, no symlink following. */
static void demo_sec6(void) {
    char dir_tmpl[] = "sec6_demo_XXXXXX";
    char *dir = mkdtemp(dir_tmpl);
    assert(dir != NULL);

    char path[64];
    int n = snprintf(path, sizeof path, "%s/out.dat", dir);
    assert(n > 0 && (size_t)n < sizeof path);

    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    assert(fd >= 0);                       /* created atomically, or we'd know */
    int fd2 = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);  /* outside assert */
    assert(fd2 == -1 && errno == EEXIST);  /* O_EXCL refuses a second create */
    (void)close(fd);                       /* best-effort demo cleanup */
    (void)unlink(path);
    (void)rmdir(dir);
}

/* SEC7: wipe secrets through a volatile pointer the optimizer can't elide. */
static void secure_wipe(void *buf, size_t len) {
    volatile unsigned char *p = buf;
    for (size_t i = 0; i < len; i++) {
        p[i] = 0;
    }
}
static void demo_sec7(void) {
    char secret[16];
    memcpy(secret, "hunter2", 8);
    secure_wipe(secret, sizeof secret);
    for (size_t i = 0; i < sizeof secret; i++) {
        assert(secret[i] == 0);
    }
}

/* SEC8: rand() only for non-security uses (simulation, tests). */
static void demo_sec8(void) {
    srand(12345);                          /* deterministic test seed — fine HERE */
    for (int i = 0; i < 100; i++) {
        int die = rand() % 6 + 1;          /* simulation use, never a token/key */
        assert(die >= 1 && die <= 6);
    }
}

int main(void) {
    demo_sec1(); demo_sec2(); demo_sec3(); demo_sec4();
    demo_sec5(); demo_sec6(); demo_sec7(); demo_sec8();
    puts("security: all demos OK");
    return 0;
}
