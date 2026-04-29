#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define PASS "\033[32mPASS\033[0m"
#define FAIL "\033[31mFAIL\033[0m"

static int g_pass = 0;
static int g_fail = 0;

static void check(int cond, const char* name) {
    if (cond) {
        printf("  [" PASS "] %s\n", name);
        g_pass++;
    } else {
        printf("  [" FAIL "] %s\n", name);
        g_fail++;
    }
}

static void section(const char* title) {
    printf("\n========================================\n");
    printf("  %s\n", title);
    printf("========================================\n");
}

/* ------------------------------------------------------------------ */
/*  1. printf / fprintf                                                 */
/* ------------------------------------------------------------------ */
static void test_printf(void) {
    section("1. printf / fprintf");

    printf("  hello from printf\n");
    check(1, "printf does not crash");

    fprintf(stdout, "  hello from fprintf stdout\n");
    check(1, "fprintf stdout does not crash");

    fprintf(stderr, "  hello from fprintf stderr\n");
    check(1, "fprintf stderr does not crash");

    printf("  int: %d\n", 42);
    printf("  neg: %d\n", -42);
    printf("  hex: %x\n", 0xDEAD);
    printf("  str: %s\n", "test");
    printf("  chr: %c\n", 'X');
    printf("  flt: %f\n", 3.14);
    check(1, "printf format specifiers do not crash");
}

/* ------------------------------------------------------------------ */
/*  2. malloc / free / calloc / realloc                                */
/* ------------------------------------------------------------------ */
static void test_malloc(void) {
    section("2. musl malloc / free / calloc / realloc");

    void* p = malloc(64);
    check(p != NULL, "malloc(64) != NULL");
    if (p) {
        memset(p, 0xAA, 64);
        check(((unsigned char*)p)[63] == 0xAA, "malloc memory writable");
        free(p);
        check(1, "free does not crash");
    }

    void* p2 = calloc(16, 8);
    check(p2 != NULL, "calloc(16,8) != NULL");
    if (p2) {
        int ok = 1;
        for (int i = 0; i < 128; i++)
            if (((unsigned char*)p2)[i] != 0) { ok = 0; break; }
        check(ok, "calloc memory is zeroed");
        free(p2);
    }

    void* p3 = malloc(32);
    check(p3 != NULL, "malloc(32) for realloc");
    if (p3) {
        memset(p3, 0xBB, 32);
        void* p4 = realloc(p3, 128);
        check(p4 != NULL, "realloc to larger size");
        if (p4) {
            int ok = 1;
            for (int i = 0; i < 32; i++)
                if (((unsigned char*)p4)[i] != 0xBB) { ok = 0; break; }
            check(ok, "realloc preserves data");
            free(p4);
        }
    }

    free(NULL);
    check(1, "free(NULL) does not crash");

    /* Stres */
    void* ptrs[64];
    int ok = 1;
    for (int i = 0; i < 64; i++) {
        ptrs[i] = malloc(64 * (i + 1));
        if (!ptrs[i]) { ok = 0; break; }
        memset(ptrs[i], (char)i, 64 * (i + 1));
    }
    check(ok, "64 sequential mallocs succeed");

    int ok2 = 1;
    for (int i = 0; i < 64; i++) {
        if (!ptrs[i]) continue;
        unsigned char* pp = (unsigned char*)ptrs[i];
        for (int j = 0; j < 64 * (i + 1); j++)
            if (pp[j] != (unsigned char)i) { ok2 = 0; break; }
        if (!ok2) break;
    }
    check(ok2, "all malloc regions contain correct data");

    for (int i = 0; i < 64; i++) if (ptrs[i]) free(ptrs[i]);

    void* after = malloc(64);
    check(after != NULL, "malloc works after mass free");
    if (after) free(after);
}

/* ------------------------------------------------------------------ */
/*  3. string funkcije                                                  */
/* ------------------------------------------------------------------ */
static void test_strings(void) {
    section("3. musl string functions");

    check(strlen("hello") == 5, "strlen");
    check(strcmp("abc", "abc") == 0, "strcmp equal");
    check(strcmp("abc", "abd") < 0,  "strcmp less");
    check(strcmp("abd", "abc") > 0,  "strcmp greater");

    char buf[32];
    strcpy(buf, "hello");
    check(strcmp(buf, "hello") == 0, "strcpy");

    strcat(buf, " world");
    check(strcmp(buf, "hello world") == 0, "strcat");

    char* p = strchr("hello", 'l');
    check(p != NULL && *p == 'l', "strchr found");
    check(strchr("hello", 'z') == NULL, "strchr not found");

    char* s = strdup("test string");
    check(s != NULL, "strdup != NULL");
    check(strcmp(s, "test string") == 0, "strdup content correct");
    free(s);

    char dst[16];
    snprintf(dst, sizeof(dst), "%d-%s", 42, "ok");
    check(strcmp(dst, "42-ok") == 0, "snprintf");

    check(atoi("42")   == 42,  "atoi positive");
    check(atoi("-7")   == -7,  "atoi negative");
    check(atol("99999") == 99999L, "atol");
    check(atof("3.14") > 3.13 && atof("3.14") < 3.15, "atof");
}

/* ------------------------------------------------------------------ */
/*  4. file I/O preko musl                                             */
/* ------------------------------------------------------------------ */
static void test_file_io(void) {
    section("4. musl file I/O (fopen/fclose/fread/fwrite)");

    /* fopen write */
    FILE* f = fopen("/musl_test.txt", "w");
    check(f != NULL, "fopen write");
    if (f) {
        int n = fprintf(f, "hello musl file\n");
        check(n > 0, "fprintf to file");
        fclose(f);
    }

    /* fopen read */
    f = fopen("/musl_test.txt", "r");
    check(f != NULL, "fopen read");
    if (f) {
        char buf[64];
        char* r = fgets(buf, sizeof(buf), f);
        check(r != NULL, "fgets returns non-null");
        check(strcmp(buf, "hello musl file\n") == 0, "fgets content correct");
        fclose(f);
    }

    /* fopen nonexistent */
    f = fopen("/nonexistent_xyz.txt", "r");
    check(f == NULL, "fopen nonexistent returns NULL");
    if (f) fclose(f);

    /* fread / fwrite */
    f = fopen("/musl_rw.bin", "wb");
    check(f != NULL, "fopen wb");
    if (f) {
        unsigned char wbuf[256];
        for (int i = 0; i < 256; i++) wbuf[i] = (unsigned char)i;
        size_t written = fwrite(wbuf, 1, 256, f);
        check(written == 256, "fwrite 256 bytes");
        fclose(f);
    }

    f = fopen("/musl_rw.bin", "rb");
    check(f != NULL, "fopen rb");
    if (f) {
        unsigned char rbuf[256];
        size_t r = fread(rbuf, 1, 256, f);
        check(r == 256, "fread 256 bytes");
        int ok = 1;
        for (int i = 0; i < 256; i++)
            if (rbuf[i] != (unsigned char)i) { ok = 0; break; }
        check(ok, "fread data matches fwrite");
        fclose(f);
    }

    /* fseek / ftell */
    f = fopen("/musl_test.txt", "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        check(sz > 0, "ftell after SEEK_END > 0");
        fseek(f, 0, SEEK_SET);
        check(ftell(f) == 0, "ftell after SEEK_SET == 0");
        fclose(f);
    }
}

/* ------------------------------------------------------------------ */
/*  5. errno                                                            */
/* ------------------------------------------------------------------ */
static void test_errno(void) {
    section("5. errno");

    errno = 0;
    FILE* f = fopen("/no_such_file_xyz", "r");
    check(f == NULL,  "fopen nonexistent fails");
    check(errno != 0, "errno set after failed fopen");
    check(errno == ENOENT, "errno == ENOENT");
    printf("  errno=%d strerror=%s\n", errno, strerror(errno));

    errno = 0;
    check(errno == 0, "errno cleared manually");
}

/* ------------------------------------------------------------------ */
/*  6. fork + musl                                                      */
/* ------------------------------------------------------------------ */
static void test_fork_musl(void) {
    section("6. fork + musl malloc in child");

    pid_t child = fork();
    if (child == 0) {
        void* p = malloc(1024);
        if (!p) exit(1);
        memset(p, 0xCC, 1024);
        free(p);
        printf("  child malloc ok\n");
        exit(0);
    }

    int status = 0;
    pid_t w = waitpid(child, &status, 0);
    check(w == child, "waitpid returns child");
    check(status == 0, "child exit 0 after malloc");

    /* Parent malloc nije korumpiran */
    void* p = malloc(1024);
    check(p != NULL, "parent malloc ok after fork");
    if (p) free(p);
}

/* ------------------------------------------------------------------ */
/*  7. pthread basic                                                    */
/* ------------------------------------------------------------------ */
#include <pthread.h>

static volatile int g_thread_ran = 0;

static void* thread_func(void* arg) {
    g_thread_ran = 1;
    void* p = malloc(256);
    if (p) {
        memset(p, 0xAB, 256);
        free(p);
    }
    return (void*)42;
}

static void test_pthread(void) {
    section("7. pthread_create / pthread_join");

    g_thread_ran = 0;
    pthread_t t;
    int r = pthread_create(&t, NULL, thread_func, NULL);
    check(r == 0, "pthread_create returns 0");

    void* retval = NULL;
    r = pthread_join(t, &retval);
    check(r == 0, "pthread_join returns 0");
    check(g_thread_ran == 1, "thread function executed");
    check((long)retval == 42, "thread return value correct");
}

/* ------------------------------------------------------------------ */
/*  8. pthread mutex                                                    */
/* ------------------------------------------------------------------ */
static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER;
static volatile int    g_count = 0;

static void* thread_increment(void* arg) {
    int n = *(int*)arg;
    for (int i = 0; i < n; i++) {
        pthread_mutex_lock(&g_mtx);
        g_count++;
        pthread_mutex_unlock(&g_mtx);
    }
    return NULL;
}

static void test_pthread_mutex(void) {
    section("8. pthread mutex");

    g_count = 0;
    int iters = 500;
    const int NT = 4;
    pthread_t threads[4];

    for (int i = 0; i < NT; i++)
        pthread_create(&threads[i], NULL, thread_increment, &iters);
    for (int i = 0; i < NT; i++)
        pthread_join(threads[i], NULL);

    check(g_count == NT * iters, "mutex: no lost updates");
    printf("  count=%d expected=%d\n", g_count, NT * iters);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */
int main(void) {
    printf("\n*** MUSL LIBC TEST SUITE ***\n");
    printf("    pid=%d\n", getpid());

    test_printf();
    test_malloc();
    test_strings();
    test_file_io();
    test_errno();
    test_fork_musl();
    test_pthread();
    test_pthread_mutex();

    printf("\n========================================\n");
    printf("  SUMMARY: %d passed, %d failed\n", g_pass, g_fail);
    printf("========================================\n");

    return g_fail > 0 ? 1 : 0;
}