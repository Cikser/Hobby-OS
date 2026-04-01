#include "syscall.h"

static void print_num(long v) {
    char buf[24];
    int i = 23;
    buf[i] = '\0';
    if (v == 0) { write(STDOUT_FILENO, "0", 1); return; }
    int neg = (v < 0);
    if (neg) v = -v;
    while (v) { buf[--i] = '0' + (int)(v % 10); v /= 10; }
    if (neg)  buf[--i] = '-';
    write(STDOUT_FILENO, buf + i, 23 - i);
}

static void demo_anon_mmap(void) {
    printf("=== anonymous mmap ===\n");

    void* mem = mmap(0, 2 * 4096,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1, 0);
    if (mem == MAP_FAILED) {
        printf("mmap failed\n");
        return;
    }
    printf("mmap -> %p\n", mem);

    unsigned char* p = (unsigned char*)mem;
    for (int i = 0; i < 16; i++) p[i] = (unsigned char)i;

    int ok = 1;
    for (int i = 0; i < 16; i++)
        if (p[i] != (unsigned char)i) { ok = 0; break; }
    printf("pattern check: %s\n", ok ? "PASS" : "FAIL");

    if (mprotect(mem, 4096, PROT_READ) == 0)
        printf("mprotect PROT_READ: OK\n");
    else
        printf("mprotect PROT_READ: FAIL\n");

    mprotect(mem, 4096, PROT_READ | PROT_WRITE);

    if (munmap(mem, 2 * 4096) == 0)
        printf("munmap: OK\n");
    else
        printf("munmap: FAIL\n");
}

static void demo_file_mmap(void) {
    printf("=== file-backed mmap ===\n");

    int fd = open("/readme.txt", O_RDONLY);
    if (fd < 0) {
        printf("open /readme.txt failed\n");
        return;
    }

    void* mem = mmap(0, 4096,
                     PROT_READ,
                     MAP_PRIVATE,
                     fd, 0);
    close(fd);

    if (mem == MAP_FAILED) {
        printf("file mmap failed\n");
        return;
    }
    printf("file mmap -> %p\n", mem);

    const char* s = (const char*)mem;
    int len = 0;
    while (len < 32 && s[len] != '\0' && s[len] != '\n') len++;
    write(STDOUT_FILENO, "content: ", 9);
    write(STDOUT_FILENO, s, len);
    write(STDOUT_FILENO, "\n", 1);

    munmap(mem, 4096);
    printf("file mmap unmapped\n");
}

static void demo_fixed_mmap(void) {
    printf("=== MAP_FIXED mmap ===\n");

    void* base = mmap(0, 4096,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (base == MAP_FAILED) { printf("base alloc failed\n"); return; }
    munmap(base, 4096);

    void* fixed = mmap(base, 4096,
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (fixed == MAP_FAILED) {
        printf("MAP_FIXED failed\n");
        return;
    }
    printf("MAP_FIXED addr matches: %s\n",
           (fixed == base) ? "PASS" : "FAIL");

    *(int*)fixed = 0xDEADBEEF;
    printf("write to fixed page: PASS\n");
    munmap(fixed, 4096);
}

void _start(void) {
    printf("Hello from userspace! pid=");
    print_num(getpid());
    printf("\n");

    pid_t child = fork();

    if (child == 0) {
        demo_anon_mmap();
        demo_file_mmap();
        demo_fixed_mmap();

        printf("child done, exiting\n");
        exit(0);
    } else {
        int status = 0;
        waitpid(child, &status);
        printf("parent: child exited, status=%d\n", WEXITSTATUS(status));
    }

    exit(0);
}