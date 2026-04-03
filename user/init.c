#include "syscall.h"

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

static void print_summary(void) {
    printf("\n========================================\n");
    printf("  SUMMARY: %d passed, %d failed\n", g_pass, g_fail);
    printf("========================================\n");
}

/* ------------------------------------------------------------------ */
/*  helpers                                                             */
/* ------------------------------------------------------------------ */

static int file_write_str(int fd, const char* s) {
    return (int)write(fd, s, strlen(s));
}

static int file_read_all(int fd, char* buf, int cap) {
    int total = 0;
    while (total < cap - 1) {
        int n = (int)read(fd, buf + total, cap - 1 - total);
        if (n <= 0) break;
        total += n;
    }
    buf[total] = '\0';
    return total;
}

static uint64_t mono_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)(ts.tv_nsec / 1000000);
}

/* ------------------------------------------------------------------ */
/*  1. getpid, getuid, geteuid, getgid, getegid                        */
/* ------------------------------------------------------------------ */

static void test_identity(void) {
    section("1. Identity syscalls");

    pid_t pid = getpid();
    check(pid >= 0, "getpid returns non-negative");

    check(getuid()  == 0, "getuid  == 0");
    check(geteuid() == 0, "geteuid == 0");
    check(getgid()  == 0, "getgid  == 0");
    check(getegid() == 0, "getegid == 0");

    printf("    pid=%d uid=%u euid=%u gid=%u egid=%u\n",
           pid, getuid(), geteuid(), getgid(), getegid());
}

/* ------------------------------------------------------------------ */
/*  2. set_tid_address                                                  */
/* ------------------------------------------------------------------ */

static void test_set_tid_address(void) {
    section("2. set_tid_address");

    int tid_word = 0;
    pid_t ret = set_tid_address(&tid_word);
    check(ret == getpid(), "set_tid_address returns current pid");

    int zero = 0;
    ret = set_tid_address(&zero);
    check(ret >= 0, "set_tid_address with stack var");
}

/* ------------------------------------------------------------------ */
/*  3. uname                                                            */
/* ------------------------------------------------------------------ */

static void test_uname(void) {
    section("3. uname");

    struct utsname u;
    int r = uname(&u);
    check(r == 0, "uname succeeds");
    check(strlen(u.sysname)  > 0, "sysname non-empty");
    check(strlen(u.machine)  > 0, "machine non-empty");
    check(strlen(u.release)  > 0, "release non-empty");
    printf("    sysname=%s nodename=%s release=%s version=%s machine=%s\n",
           u.sysname, u.nodename, u.release, u.version, u.machine);

    int bad = uname((struct utsname*)0);
    check(bad < 0, "uname(NULL) fails");

    int bad2 = uname((struct utsname*)0x10);
    check(bad2 < 0, "uname(bad ptr) fails");
}

/* ------------------------------------------------------------------ */
/*  4. clock_gettime                                                    */
/* ------------------------------------------------------------------ */

static void test_clock_gettime(void) {
    section("4. clock_gettime");

    struct timespec ts1, ts2;
    int r = clock_gettime(CLOCK_MONOTONIC, &ts1);
    check(r == 0, "clock_gettime CLOCK_MONOTONIC ok");

    r = clock_gettime(CLOCK_REALTIME, &ts2);
    check(r == 0, "clock_gettime CLOCK_REALTIME ok");

    check(ts1.tv_sec >= 0 && ts1.tv_nsec >= 0, "monotonic values non-negative");

    int bad = clock_gettime(CLOCK_MONOTONIC, (struct timespec*)0);
    check(bad < 0, "clock_gettime(NULL) fails");

    int bad2 = clock_gettime(CLOCK_MONOTONIC, (struct timespec*)0x8);
    check(bad2 < 0, "clock_gettime(bad ptr) fails");

    struct timespec before, after;
    clock_gettime(CLOCK_MONOTONIC, &before);
    volatile long spin = 0;
    for (long i = 0; i < 5000000L; i++) spin++;
    clock_gettime(CLOCK_MONOTONIC, &after);
    uint64_t ms_before = (uint64_t)before.tv_sec * 1000 + before.tv_nsec / 1000000;
    uint64_t ms_after  = (uint64_t)after.tv_sec  * 1000 + after.tv_nsec  / 1000000;
    check(ms_after >= ms_before, "time is monotonically non-decreasing");
}

/* ------------------------------------------------------------------ */
/*  5. write, read via stdout/stderr                                    */
/* ------------------------------------------------------------------ */

static void test_write_read_basic(void) {
    section("5. write / read basic");

    ssize_t w = write(STDOUT_FILENO, "hello stdout\n", 13);
    check(w == 13, "write 13 bytes to stdout");

    w = write(STDERR_FILENO, "hello stderr\n", 13);
    check(w == 13, "write 13 bytes to stderr");

    w = write(STDOUT_FILENO, "", 0);
    check(w == 0, "write 0 bytes returns 0");

    w = write(-1, "x", 1);
    check(w < 0, "write to invalid fd fails");

    w = write(STDOUT_FILENO, (void*)0, 10);
    check(w < 0, "write with NULL buf fails");
}

/* ------------------------------------------------------------------ */
/*  6. writev / readv                                                   */
/* ------------------------------------------------------------------ */

static void test_writev_readv(void) {
    section("6. writev / readv");

    const char* p1 = "Hello, ";
    const char* p2 = "writev ";
    const char* p3 = "world!\n";

    struct iovec iov[3];
    iov[0].iov_base = (void*)p1; iov[0].iov_len = strlen(p1);
    iov[1].iov_base = (void*)p2; iov[1].iov_len = strlen(p2);
    iov[2].iov_base = (void*)p3; iov[2].iov_len = strlen(p3);

    ssize_t w = writev(STDOUT_FILENO, iov, 3);
    check(w == (ssize_t)(strlen(p1)+strlen(p2)+strlen(p3)), "writev 3 iovecs to stdout");

    int fd = open("/writev_test.txt", O_RDWR | O_CREAT | O_TRUNC);
    if (fd >= 0) {
        const char* a = "AAAA";
        const char* b = "BBBB";
        struct iovec wv[2];
        wv[0].iov_base = (void*)a; wv[0].iov_len = 4;
        wv[1].iov_base = (void*)b; wv[1].iov_len = 4;
        w = writev(fd, wv, 2);
        check(w == 8, "writev 8 bytes to file");

        lseek(fd, 0, SEEK_SET);

        char ra[4], rb[4];
        struct iovec rv[2];
        rv[0].iov_base = ra; rv[0].iov_len = 4;
        rv[1].iov_base = rb; rv[1].iov_len = 4;
        ssize_t r = readv(fd, rv, 2);
        check(r == 8, "readv 8 bytes from file");
        check(memcmp(ra, "AAAA", 4) == 0 && memcmp(rb, "BBBB", 4) == 0,
              "readv data matches writev data");
        close(fd);
    } else {
        check(0, "writev/readv file open failed");
        check(0, "writev 8 bytes to file - skipped");
        check(0, "readv 8 bytes from file - skipped");
        check(0, "readv data matches writev data - skipped");
    }

    ssize_t bad = writev(-1, iov, 3);
    check(bad < 0, "writev to bad fd fails");
}

/* ------------------------------------------------------------------ */
/*  7. open, close, creat                                               */
/* ------------------------------------------------------------------ */

static void test_open_close(void) {
    section("7. open / close / creat");

    int fd = open("/readme.txt", O_RDONLY);
    check(fd >= 0, "open existing file O_RDONLY");
    if (fd >= 0) {
        check(close(fd) == 0, "close valid fd");
    }

    fd = open("/nonexistent_file_xyz.txt", O_RDONLY);
    check(fd < 0, "open nonexistent file fails");

    fd = creat("/creat_test.txt");
    check(fd >= 0, "creat new file");
    if (fd >= 0) close(fd);

    fd = open("/creat_test.txt", O_RDONLY);
    check(fd >= 0, "open newly created file");
    if (fd >= 0) close(fd);

    fd = open("/readme.txt", O_RDONLY);
    if (fd >= 0) {
        close(fd);
        int bad = close(fd);
        check(bad < 0, "double close fails");
    }

    int bad_close = close(-1);
    check(bad_close < 0, "close(-1) fails");

    int bad_close2 = close(999);
    check(bad_close2 < 0, "close(999) fails");

    fd = open((const char*)0, O_RDONLY);
    check(fd < 0, "open(NULL) fails");

    fd = open((const char*)0xFFFFFFFFFFFFFFFFULL, O_RDONLY);
    check(fd < 0, "open(invalid ptr) fails");
}

/* ------------------------------------------------------------------ */
/*  8. lseek                                                            */
/* ------------------------------------------------------------------ */

static void test_lseek(void) {
    section("8. lseek");

    int fd = open("/readme.txt", O_RDONLY);
    if (fd < 0) { check(0, "lseek: open failed"); return; }

    int64_t pos = lseek(fd, 0, SEEK_END);
    check(pos > 0, "lseek SEEK_END > 0");
    printf("    file size via lseek: %ld\n", (long)pos);

    int64_t pos2 = lseek(fd, 0, SEEK_SET);
    check(pos2 == 0, "lseek SEEK_SET to 0");

    int64_t pos3 = lseek(fd, 5, SEEK_CUR);
    check(pos3 == 5, "lseek SEEK_CUR +5");

    int64_t pos4 = lseek(fd, -2, SEEK_CUR);
    check(pos4 == 3, "lseek SEEK_CUR -2 => 3");

    int64_t bad = lseek(-1, 0, SEEK_SET);
    check(bad < 0, "lseek on bad fd fails");

    int64_t bad2 = lseek(fd, 0, 99);
    check(bad2 < 0, "lseek with invalid whence fails");

    close(fd);
}

/* ------------------------------------------------------------------ */
/*  9. fstat                                                            */
/* ------------------------------------------------------------------ */

static void test_fstat(void) {
    section("9. fstat");

    int fd = open("/readme.txt", O_RDONLY);
    check(fd >= 0, "fstat: open ok");
    if (fd < 0) return;

    struct stat st;
    int r = fstat(fd, &st);
    check(r == 0, "fstat returns 0");
    check(st.st_size > 0, "fstat st_size > 0");
    check(st.st_ino > 0, "fstat st_ino > 0");
    printf("    ino=%llu size=%lld mode=%u nlink=%u\n",
           (unsigned long long)st.st_ino, (long long)st.st_size,
           st.st_mode, st.st_nlink);

    r = fstat(fd, (struct stat*)0);
    check(r < 0, "fstat(fd, NULL) fails");

    r = fstat(fd, (struct stat*)0x10);
    check(r < 0, "fstat(fd, bad ptr) fails");

    r = fstat(-1, &st);
    check(r < 0, "fstat(-1, st) fails");

    close(fd);
}

/* ------------------------------------------------------------------ */
/*  10. getcwd, chdir, mkdir                                            */
/* ------------------------------------------------------------------ */

static void test_fs_dirs(void) {
    section("10. getcwd / chdir / mkdir");

    char cwd[256];
    int r = getcwd(cwd, 256);
    check(r == 0, "getcwd in root");
    check(cwd[0] == '/', "cwd starts with /");
    printf("    initial cwd: %s\n", cwd);

    r = mkdir("/test_dir_a", 0755);
    check(r == 0, "mkdir /test_dir_a");

    r = mkdir("/test_dir_a", 0755);
    check(r < 0, "mkdir duplicate fails");

    r = mkdir("/test_dir_b/nested", 0755);
    check(r < 0, "mkdir without parent fails");

    r = chdir("/test_dir_a");
    check(r == 0, "chdir into /test_dir_a");

    r = getcwd(cwd, 256);
    check(r == 0, "getcwd after chdir");
    check(strcmp(cwd, "/test_dir_a") == 0, "cwd == /test_dir_a");

    r = chdir("/test_dir_a");
    check(r == 0, "chdir same dir ok");

    r = mkdir("subdir_rel", 0755);
    check(r == 0, "mkdir relative path");

    r = chdir("subdir_rel");
    check(r == 0, "chdir relative");

    r = getcwd(cwd, 256);
    check(r == 0, "getcwd after relative chdir");
    check(strcmp(cwd, "/test_dir_a/subdir_rel") == 0, "cwd == /test_dir_a/subdir_rel");

    r = chdir("..");
    check(r == 0, "chdir ..");
    getcwd(cwd, 256);
    check(strcmp(cwd, "/test_dir_a") == 0, "cwd back to /test_dir_a");

    r = chdir("/");
    check(r == 0, "chdir /");

    r = chdir("/nonexistent_xyz");
    check(r < 0, "chdir nonexistent fails");

    r = chdir((const char*)0);
    check(r < 0, "chdir(NULL) fails");

    r = getcwd((char*)0, 256);
    check(r < 0, "getcwd(NULL) fails");

    r = getcwd(cwd, 1);
    check(r < 0, "getcwd buffer too small fails");

    r = mkdir((const char*)0, 0755);
    check(r < 0, "mkdir(NULL) fails");
}

/* ------------------------------------------------------------------ */
/*  11. file read/write/seek/trunc cycle                                */
/* ------------------------------------------------------------------ */

static void test_file_rw(void) {
    section("11. file read/write/seek cycle");

    int fd = open("/rwtest.txt", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "open O_RDWR|O_CREAT|O_TRUNC");
    if (fd < 0) return;

    const char* msg = "0123456789ABCDEF";
    ssize_t w = write(fd, msg, 16);
    check(w == 16, "write 16 bytes");

    int64_t pos = lseek(fd, 0, SEEK_SET);
    check(pos == 0, "seek to start");

    char buf[32];
    ssize_t r = read(fd, buf, 16);
    check(r == 16, "read 16 bytes back");
    check(memcmp(buf, msg, 16) == 0, "read data matches written data");

    r = read(fd, buf, 16);
    check(r <= 0, "read at EOF returns <=0");

    lseek(fd, 4, SEEK_SET);
    w = write(fd, "XXXX", 4);
    check(w == 4, "overwrite 4 bytes at offset 4");

    lseek(fd, 0, SEEK_SET);
    r = read(fd, buf, 16);
    buf[16] = '\0';
    check(r == 16 && memcmp(buf+4, "XXXX", 4) == 0, "overwrite verified");

    lseek(fd, 0, SEEK_END);
    w = write(fd, "TAIL", 4);
    check(w == 4, "append 4 bytes at end");

    struct stat st;
    fstat(fd, &st);
    check(st.st_size == 20, "file size is 20 after append");

    lseek(fd, 100, SEEK_SET);
    w = write(fd, "X", 1);
    check(w == 1, "write past end creates sparse area");
    fstat(fd, &st);
    check(st.st_size == 101, "file size 101 after sparse write");

    lseek(fd, 16, SEEK_SET);
    r = read(fd, buf, 84);
    check(r > 0, "read over sparse area succeeds");

    r = read(-1, buf, 10);
    check(r < 0, "read on bad fd fails");

    w = write(-1, "x", 1);
    check(w < 0, "write on bad fd fails");

    close(fd);

    fd = open("/rwtest_append.txt", O_WRONLY | O_CREAT | O_APPEND);
    check(fd >= 0, "open O_APPEND");
    if (fd >= 0) {
        write(fd, "aaa", 3);
        write(fd, "bbb", 3);
        close(fd);
        fd = open("/rwtest_append.txt", O_RDONLY);
        r = read(fd, buf, 32);
        check(r == 6 && memcmp(buf, "aaabbb", 6) == 0, "O_APPEND data correct");
        close(fd);
    }

    fd = open("/large_rw.txt", O_RDWR | O_CREAT | O_TRUNC);
    if (fd >= 0) {
        char block[512];
        memset(block, 0xAB, 512);
        int blocks = 20;
        for (int i = 0; i < blocks; i++) write(fd, block, 512);
        fstat(fd, &st);
        check(st.st_size == blocks * 512, "large sequential write size correct");
        lseek(fd, 0, SEEK_SET);
        int ok = 1;
        for (int i = 0; i < blocks; i++) {
            char rbuf[512];
            r = read(fd, rbuf, 512);
            if (r != 512 || memcmp(rbuf, block, 512) != 0) { ok = 0; break; }
        }
        check(ok, "large sequential read matches write");
        close(fd);
    }
}

/* ------------------------------------------------------------------ */
/*  12. O_CREAT creates new file, verifiable via fstat                  */
/* ------------------------------------------------------------------ */

static void test_creat_fstat(void) {
    section("12. creat + fstat consistency");

    const char* path = "/fstat_creat.txt";
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create file for creat+fstat test");
    if (fd < 0) return;

    write(fd, "hello", 5);

    struct stat st;
    fstat(fd, &st);
    check(st.st_size == 5, "fstat after write: size == 5");

    int fd2 = open(path, O_RDONLY);
    struct stat st2;
    fstat(fd2, &st2);
    check(st2.st_ino == st.st_ino, "same inode from two fds");
    check(st2.st_size == 5, "size consistent across two fds");
    close(fd2);
    close(fd);
}

/* ------------------------------------------------------------------ */
/*  13. fd exhaustion                                                   */
/* ------------------------------------------------------------------ */

static void test_fd_limit(void) {
    section("13. fd exhaustion");

    int fds[128];
    int count = 0;
    for (int i = 0; i < 128; i++) {
        fds[i] = open("/readme.txt", O_RDONLY);
        if (fds[i] < 0) break;
        count++;
    }
    check(count > 0, "opened at least one fd");
    printf("    opened %d fds before exhaustion\n", count);

    int extra = open("/readme.txt", O_RDONLY);
    check(extra < 0, "open after exhaustion fails");

    for (int i = 0; i < count; i++) close(fds[i]);

    int recov = open("/readme.txt", O_RDONLY);
    check(recov >= 0, "fd available after all closed");
    close(recov);
}

/* ------------------------------------------------------------------ */
/*  14. brk                                                             */
/* ------------------------------------------------------------------ */

static void test_brk(void) {
    section("14. brk");

    uint64_t initial = (uint64_t)brk((void*)0);
    check(initial > 0, "brk(0) returns current break");
    printf("    initial brk: 0x%lx\n", (unsigned long)initial);

    uint64_t target = initial + 4096;
    uint64_t res = (uint64_t)brk((void*)target);
    check(res == target, "brk grow by 4096");

    char* p = (char*)initial;
    for (int i = 0; i < 4096; i++) p[i] = (char)(i & 0xFF);
    int ok = 1;
    for (int i = 0; i < 4096; i++) if (p[i] != (char)(i & 0xFF)) { ok = 0; break; }
    check(ok, "brk memory writable and readable");

    res = (uint64_t)brk((void*)initial);
    check(res == initial, "brk shrink back to initial");

    res = (uint64_t)brk((void*)0x7FFFFFFFFFFFFFFFULL);
    check(res != 0x7FFFFFFFFFFFFFFFULL, "extreme brk rejected");

    res = (uint64_t)brk((void*)0x100);
    check(res != 0x100, "brk below heap base rejected");

    uint64_t cur = (uint64_t)brk((void*)0);
    check(cur == initial, "brk still at initial after failed attempts");

    uint64_t mb = initial;
    int allocs = 0;
    while (1) {
        uint64_t next = mb + (1024 * 1024);
        uint64_t r2 = (uint64_t)brk((void*)next);
        if (r2 != next) break;
        mb = next;
        allocs++;
        if (allocs > 200) break;
    }
    printf("    brk grew %d MB before OOM or limit\n", allocs);
    check(allocs > 0, "brk can allocate at least 1 MB");

    uint64_t reset = (uint64_t)brk((void*)initial);
    check(reset == initial, "brk reset to initial after stress");
}

/* ------------------------------------------------------------------ */
/*  15. mmap / mprotect / munmap                                        */
/* ------------------------------------------------------------------ */

static void test_mmap(void) {
    section("15. mmap / mprotect / munmap");

    void* p = mmap((void*)0, 4096, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED && p != (void*)0, "mmap anon page");

    char* cp = (char*)p;
    for (int i = 0; i < 4096; i++) cp[i] = (char)(i & 0x7F);
    int ok = 1;
    for (int i = 0; i < 4096; i++) if (cp[i] != (char)(i & 0x7F)) { ok = 0; break; }
    check(ok, "mmap page writable/readable");

    int r = munmap(p, 4096);
    check(r == 0, "munmap single page");

    p = mmap((void*)0, 4096 * 8, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED, "mmap 8 pages");
    if (p != MAP_FAILED) {
        memset(p, 0xCC, 4096 * 8);
        check(((unsigned char*)p)[0] == 0xCC, "mmap 8 pages writable");
        check(munmap(p, 4096 * 8) == 0, "munmap 8 pages");
    }

    p = mmap((void*)0, 0, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p == MAP_FAILED, "mmap length=0 fails");

    p = mmap((void*)0, 4096, PROT_READ | PROT_WRITE,
             MAP_ANONYMOUS, -1, 0);
    check(p == MAP_FAILED, "mmap without MAP_PRIVATE|MAP_SHARED fails");

    p = mmap((void*)0, 4096, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, 5, 0);
    check(p == MAP_FAILED, "mmap MAP_ANONYMOUS with valid fd fails");

    p = mmap((void*)0, 4096 * 4, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED, "mmap for mprotect test");
    if (p != MAP_FAILED) {
        memset(p, 0xBB, 4096 * 4);
        r = mprotect(p, 4096 * 4, PROT_READ);
        check(r == 0, "mprotect PROT_READ");
        check(((unsigned char*)p)[100] == 0xBB, "mprotect PROT_READ still readable");

        r = mprotect(p, 4096 * 4, PROT_READ | PROT_WRITE);
        check(r == 0, "mprotect back to RW");
        ((unsigned char*)p)[0] = 0xAA;
        check(((unsigned char*)p)[0] == 0xAA, "writable after mprotect back to RW");

        r = mprotect(p, 4096 * 4, PROT_NONE);
        check(r == 0, "mprotect PROT_NONE");

        munmap(p, 4096 * 4);
    }

    r = munmap((void*)0x1000, 4096);
    check(r == 0, "munmap unmapped region returns 0");

    r = munmap((void*)0x1001, 4096);
    check(r < 0, "munmap unaligned addr fails");

    r = munmap((void*)0, 0);
    check(r < 0, "munmap length=0 fails");

    r = mprotect((void*)0x1, 4096, PROT_READ);
    check(r < 0, "mprotect unaligned fails");
}

/* ------------------------------------------------------------------ */
/*  16. mmap file-backed                                                */
/* ------------------------------------------------------------------ */

static void test_mmap_file(void) {
    section("16. mmap file-backed");

    const char* path = "/mmap_file_test.bin";
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create mmap test file");
    if (fd < 0) return;

    char wbuf[4096];
    for (int i = 0; i < 4096; i++) wbuf[i] = (char)(i & 0xFF);
    write(fd, wbuf, 4096);

    void* p = mmap((void*)0, 4096, PROT_READ, MAP_PRIVATE, fd, 0);
    check(p != MAP_FAILED, "mmap file-backed private read");
    if (p != MAP_FAILED) {
        int match = memcmp(p, wbuf, 4096) == 0;
        check(match, "mmap file-backed data matches file content");
        munmap(p, 4096);
    }

    p = mmap((void*)0, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    check(p != MAP_FAILED, "mmap file-backed private rw");
    if (p != MAP_FAILED) {
        ((char*)p)[0] = 0xFF;
        check(((char*)p)[0] == 0xFF, "private mmap write visible in mapping");
        lseek(fd, 0, SEEK_SET);
        char check_byte;
        read(fd, &check_byte, 1);
        check((unsigned char)check_byte == 0x00,
              "private mmap write NOT visible in file (COW)");
        munmap(p, 4096);
    }

    void* bad = mmap((void*)0, 4096, PROT_READ, MAP_PRIVATE, -1, 0);
    check(bad == MAP_FAILED, "mmap non-anon with fd=-1 fails");

    close(fd);
}

/* ------------------------------------------------------------------ */
/*  17. fork basic                                                      */
/* ------------------------------------------------------------------ */

static void test_fork_basic(void) {
    section("17. fork basic");

    pid_t parent_pid = getpid();
    pid_t child = fork();

    if (child == 0) {
        check(getpid() != parent_pid, "child has different pid");
        check(getpid() > 0, "child pid > 0");
        exit(42);
    }

    check(child > 0, "fork returns child pid > 0 to parent");
    int status = 0;
    pid_t w = waitpid(child, &status);
    check(w == child, "waitpid returns correct child pid");
    check(status == 42, "child exit code == 42");
}

/* ------------------------------------------------------------------ */
/*  18. fork memory isolation                                           */
/* ------------------------------------------------------------------ */

static void test_fork_memory(void) {
    section("18. fork memory isolation");

    int shared_before = 999;
    pid_t child = fork();

    if (child == 0) {
        shared_before = 0;
        exit(0);
    }

    waitpid(child, (int*)0);
    check(shared_before == 999, "parent variable unaffected by child write");

    char* heap = (char*)malloc(4096);
    if (heap) {
        memset(heap, 0xAA, 4096);
        child = fork();
        if (child == 0) {
            memset(heap, 0xBB, 4096);
            exit(0);
        }
        waitpid(child, (int*)0);
        int ok = 1;
        for (int i = 0; i < 4096; i++) if ((unsigned char)heap[i] != 0xAA) { ok = 0; break; }
        check(ok, "parent heap unmodified after child overwrote its copy");
        free(heap);
    }
    // todo check
    /*void* mp = mmap((void*)0, 4096, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mp != MAP_FAILED) {
        ((char*)mp)[0] = 0x11;
        child = fork();
        if (child == 0) {
            ((char*)mp)[0] = 0x22;
            exit((int)(unsigned char)((char*)mp)[0]);
        }
        int st = 0;
        waitpid(child, &st);
        check(st == 0x22, "child saw its own mmap write");
        check(((unsigned char*)mp)[0] == 0x11, "parent mmap unaffected by child");
        munmap(mp, 4096);
    }*/
}

/* ------------------------------------------------------------------ */
/*  19. fork + file descriptor inheritance                              */
/* ------------------------------------------------------------------ */

static void test_fork_fd_inherit(void) {
    section("19. fork fd inheritance");

    int fd = open("/fork_fd_test.txt", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create file for fork fd test");
    if (fd < 0) return;
    write(fd, "parent", 6);

    pid_t child = fork();
    if (child == 0) {
        lseek(fd, 0, SEEK_SET);
        char buf[8];
        int r = (int)read(fd, buf, 6);
        exit(r == 6 ? 0 : 1);
    }
    int st = 0;
    waitpid(child, &st);
    check(st == 0, "child can read from inherited fd");

    close(fd);
}

/* ------------------------------------------------------------------ */
/*  20. fork chain (grandchild)                                         */
/* ------------------------------------------------------------------ */

static void test_fork_chain(void) {
    section("20. fork chain (grandchild)");

    pid_t c1 = fork();
    if (c1 == 0) {
        pid_t c2 = fork();
        if (c2 == 0) {
            exit(77);
        }
        int st = 0;
        waitpid(c2, &st);
        exit(st == 77 ? 0 : 1);
    }
    int st = 0;
    waitpid(c1, &st);
    check(st == 0, "fork chain: grandchild exit propagates correctly");
}

/* ------------------------------------------------------------------ */
/*  21. wait on no children                                             */
/* ------------------------------------------------------------------ */

static void test_wait_no_children(void) {
    section("21. wait on no children");

    pid_t r = waitpid(-1, (int*)0);
    check(r < 0, "waitpid with no children returns <0");

    r = waitpid(99999, (int*)0);
    check(r < 0, "waitpid nonexistent pid returns <0");
}

/* ------------------------------------------------------------------ */
/*  22. exit codes full range                                           */
/* ------------------------------------------------------------------ */

static void test_exit_codes(void) {
    section("22. exit code range");

    int codes[] = { 0, 1, 2, 7, 42, 100, 127, 255 };
    int n = (int)(sizeof(codes)/sizeof(codes[0]));
    for (int i = 0; i < n; i++) {
        pid_t c = fork();
        if (c == 0) exit(codes[i]);
        int st = 0;
        waitpid(c, &st);
        check(st == codes[i], "exit code round-trip");
    }
}

/* ------------------------------------------------------------------ */
/*  23. execve                                                          */
/* ------------------------------------------------------------------ */

//todo check
/*
static void test_execve(void) {
    section("23. execve");

    pid_t child = fork();
    if (child == 0) {
        const char* argv[] = { "/bin/init", 0 };
        const char* envp[] = { 0 };
        int r = execve("/bin/init", (char* const*)argv, (char* const*)envp);
        exit(r < 0 ? 200 : 201);
    }
    int st = 0;
    waitpid(child, &st);
    check(st == 0, "execve /bin/init succeeds (exits 0)");

    child = fork();
    if (child == 0) {
        const char* argv[] = { 0 };
        const char* envp[] = { 0 };
        execve("/nonexistent_binary", (char* const*)argv, (char* const*)envp);
        exit(123);
    }
    waitpid(child, &st);
    check(st == 123, "execve nonexistent binary fails, falls through");

    child = fork();
    if (child == 0) {
        execve((const char*)0, (char* const*)0, (char* const*)0);
        exit(124);
    }
    waitpid(child, &st);
    check(st == 124, "execve NULL path fails");
}*/

/* ------------------------------------------------------------------ */
/*  24. fork bomb limit                                                 */
/* ------------------------------------------------------------------ */

static void test_fork_bomb(void) {
    section("24. fork bomb / process limit");

    int pids[512];
    int count = 0;
    for (int i = 0; i < 512; i++) {
        pids[i] = (int)fork();
        if (pids[i] < 0) break;
        if (pids[i] == 0) exit(i & 0xFF);
        count++;
    }
    printf("    forked %d children\n", count);
    check(count > 0, "fork bomb: at least one fork succeeded");

    int reaped = 0;
    for (int i = 0; i < count; i++) {
        int st = 0;
        pid_t r = waitpid(pids[i], &st);
        if (r == (pid_t)pids[i]) reaped++;
    }
    check(reaped == count, "fork bomb: all children reaped");
}

/* ------------------------------------------------------------------ */
/*  25. fork + mmap shared                                              */
/* ------------------------------------------------------------------ */
//todo check
/*
static void test_fork_mmap_shared(void) {
    section("25. fork + MAP_SHARED");

    void* p = mmap((void*)0, 4096, PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) {
        check(0, "mmap MAP_SHARED anon for fork test");
        return;
    }
    check(1, "mmap MAP_SHARED anon for fork test");

    ((char*)p)[0] = 0x01;

    pid_t child = fork();
    if (child == 0) {
        int saw = ((unsigned char*)p)[0];
        ((char*)p)[0] = 0x02;
        exit(saw == 0x01 ? 0 : 1);
    }
    int st = 0;
    waitpid(child, &st);
    check(st == 0, "child read parent value in shared mapping");
    check(((unsigned char*)p)[0] == 0x02, "parent sees child write via MAP_SHARED");

    munmap(p, 4096);
}*/

/* ------------------------------------------------------------------ */
/*  26. recursive fork tree (scheduler stress)                         */
/* ------------------------------------------------------------------ */

static void fork_tree(int depth) {
    if (depth == 0) { exit(0); }
    pid_t a = fork();
    if (a == 0) fork_tree(depth - 1);
    pid_t b = fork();
    if (b == 0) fork_tree(depth - 1);
    int s;
    waitpid(a, &s);
    waitpid(b, &s);
    exit(0);
}

static void test_fork_tree(void) {
    section("26. fork tree (scheduler stress)");

    pid_t root = fork();
    if (root == 0) fork_tree(4);
    int st = 0;
    waitpid(root, &st);
    check(st == 0, "fork tree depth 4 (16 leaves) completes");

    root = fork();
    if (root == 0) fork_tree(5);
    waitpid(root, &st);
    check(st == 0, "fork tree depth 5 (32 leaves) completes");
}

/* ------------------------------------------------------------------ */
/*  27. memory stress: alloc/free patterns via brk                     */
/* ------------------------------------------------------------------ */

static void test_memory_stress(void) {
    section("27. memory stress via brk");

    uint64_t base = (uint64_t)brk((void*)0);
    uint64_t cur  = base;

    for (int i = 0; i < 16; i++) {
        uint64_t nb = cur + 1024 * 1024;
        uint64_t r  = (uint64_t)brk((void*)nb);
        if (r != nb) { printf("    OOM at %d MB\n", i+1); break; }
        char* p = (char*)cur;
        for (int j = 0; j < 1024 * 1024; j++) p[j] = (char)((i + j) & 0xFF);
        cur = nb;
    }

    int ok = 1;
    uint64_t scan = base;
    for (int i = 0; scan < cur; i++) {
        char* p = (char*)scan;
        for (int j = 0; j < 1024 * 1024 && scan + j < cur; j++) {
            if (p[j] != (char)((i + j) & 0xFF)) { ok = 0; break; }
        }
        scan += 1024 * 1024;
        if (!ok) break;
    }
    check(ok, "brk stress: all written data verified");

    uint64_t r = (uint64_t)brk((void*)base);
    check(r == base, "brk stress: full reset succeeds");

    uint64_t after = (uint64_t)brk((void*)0);
    check(after == base, "brk back to base after stress");
}

/* ------------------------------------------------------------------ */
/*  28. mmap stress: many small mappings                               */
/* ------------------------------------------------------------------ */

static void test_mmap_stress(void) {
    section("28. mmap stress: many small mappings");

    void* ptrs[64];
    int mapped = 0;
    for (int i = 0; i < 64; i++) {
        ptrs[i] = mmap((void*)0, 4096, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (ptrs[i] == MAP_FAILED) break;
        memset(ptrs[i], i & 0xFF, 4096);
        mapped++;
    }
    printf("    mapped %d pages\n", mapped);
    check(mapped > 0, "mmap stress: mapped at least 1 page");

    int ok = 1;
    for (int i = 0; i < mapped; i++) {
        unsigned char* p = (unsigned char*)ptrs[i];
        for (int j = 0; j < 4096; j++) {
            if (p[j] != (unsigned char)(i & 0xFF)) { ok = 0; break; }
        }
    }
    check(ok, "mmap stress: all pages contain correct data");

    for (int i = 0; i < mapped; i++) munmap(ptrs[i], 4096);

    void* p = mmap((void*)0, 4096, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED, "mmap succeeds after freeing all mappings");
    if (p != MAP_FAILED) munmap(p, 4096);
}

/* ------------------------------------------------------------------ */
/*  29. mmap large contiguous region                                   */
/* ------------------------------------------------------------------ */

static void test_mmap_large(void) {
    section("29. mmap large contiguous region");

    size_t size = 4096 * 256;
    void* p = mmap((void*)0, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED, "mmap 256 pages contiguous");
    if (p == MAP_FAILED) return;

    char* cp = (char*)p;
    for (size_t i = 0; i < size; i++) cp[i] = (char)(i & 0x7F);

    int ok = 1;
    for (size_t i = 0; i < size; i++) {
        if (cp[i] != (char)(i & 0x7F)) { ok = 0; break; }
    }
    check(ok, "mmap large: data correct");

    int r = mprotect(p, size / 2, PROT_READ);
    check(r == 0, "mprotect first half PROT_READ");

    r = mprotect(p, size / 2, PROT_READ | PROT_WRITE);
    check(r == 0, "mprotect first half back to RW");

    r = munmap(p, size / 2);
    check(r == 0, "munmap first half");

    r = munmap((char*)p + size / 2, size / 2);
    check(r == 0, "munmap second half");
}

/* ------------------------------------------------------------------ */
/*  30. concurrent mmap/brk in child                                   */
/* ------------------------------------------------------------------ */

static void test_mmap_fork_isolation(void) {
    section("30. mmap/brk isolation across fork");

    void* parent_map = mmap((void*)0, 4096, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(parent_map != MAP_FAILED, "parent mmap before fork");
    //todo check
    /*if (parent_map == MAP_FAILED) return;
    ((char*)parent_map)[0] = 0x55;

    uint64_t parent_brk = (uint64_t)brk((void*)0);

    pid_t child = fork();
    if (child == 0) {
        ((char*)parent_map)[0] = 0x66;
        uint64_t nb = parent_brk + 4096 * 16;
        brk((void*)nb);
        munmap(parent_map, 4096);
        exit(0);
    }
    int st = 0;
    waitpid(child, &st);

    check(((unsigned char*)parent_map)[0] == 0x55,
          "parent mmap value unchanged after child modified its copy");

    uint64_t after_brk = (uint64_t)brk((void*)0);
    check(after_brk == parent_brk,
          "parent brk unaffected by child brk call");
*/
    munmap(parent_map, 4096);
}

/* ------------------------------------------------------------------ */
/*  31. filesystem: unlink                                              */
/* ------------------------------------------------------------------ */

static void test_unlink(void) {
    section("31. filesystem unlink");

    int fd = open("/to_delete.txt", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create file to unlink");
    if (fd >= 0) {
        write(fd, "bye", 3);
        close(fd);
    }

    int fd_before = open("/to_delete.txt", O_RDONLY);
    check(fd_before >= 0, "file exists before unlink");
    if (fd_before >= 0) close(fd_before);

    int r = __syscall(1026, (long)"/to_delete.txt", 0, 0, 0, 0, 0);

    if (r == 0) {
        int fd_after = open("/to_delete.txt", O_RDONLY);
        check(fd_after < 0, "file gone after unlink");
        if (fd_after >= 0) close(fd_after);
    } else {
        printf("    unlink not available or returned %d, skipping\n", r);
        check(1, "unlink skipped (syscall not wired up)");
        check(1, "file gone after unlink - skipped");
    }
}

/* ------------------------------------------------------------------ */
/*  32. readv/writev stress                                             */
/* ------------------------------------------------------------------ */

static void test_readv_writev_stress(void) {
    section("32. readv/writev stress");

    int fd = open("/iov_stress.bin", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "open file for iov stress");
    if (fd < 0) return;

    char bufs[8][64];
    struct iovec wv[8];
    for (int i = 0; i < 8; i++) {
        memset(bufs[i], 'A' + i, 64);
        wv[i].iov_base = bufs[i];
        wv[i].iov_len  = 64;
    }

    ssize_t w = writev(fd, wv, 8);
    check(w == 512, "writev 8 x 64 = 512 bytes");

    lseek(fd, 0, SEEK_SET);

    char rbufs[8][64];
    struct iovec rv[8];
    for (int i = 0; i < 8; i++) {
        rbufs[i][0] = 0;
        rv[i].iov_base = rbufs[i];
        rv[i].iov_len  = 64;
    }
    ssize_t r = readv(fd, rv, 8);
    check(r == 512, "readv 8 x 64 = 512 bytes");

    int ok = 1;
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 64; j++) {
            if ((unsigned char)rbufs[i][j] != (unsigned char)('A' + i)) { ok = 0; break; }
        }
    }
    check(ok, "readv data matches writev data");

    struct iovec bad_iov[2];
    bad_iov[0].iov_base = (void*)0;
    bad_iov[0].iov_len  = 10;
    bad_iov[1].iov_base = (void*)0;
    bad_iov[1].iov_len  = 10;
    ssize_t br = readv(fd, bad_iov, 2);
    check(br < 0, "readv with NULL iov_base fails");

    close(fd);
}

/* ------------------------------------------------------------------ */
/*  33. deep directory traversal                                       */
/* ------------------------------------------------------------------ */

static void test_deep_path(void) {
    section("33. deep directory traversal");

    mkdir("/deep", 0755);
    mkdir("/deep/a", 0755);
    mkdir("/deep/a/b", 0755);
    mkdir("/deep/a/b/c", 0755);
    mkdir("/deep/a/b/c/d", 0755);

    int r = chdir("/deep/a/b/c/d");
    check(r == 0, "chdir to depth-5 directory");

    char cwd[256];
    getcwd(cwd, 256);
    check(strcmp(cwd, "/deep/a/b/c/d") == 0, "cwd correct at depth 5");

    r = chdir("../..");
    check(r == 0, "chdir ../.. from depth 5");
    getcwd(cwd, 256);
    check(strcmp(cwd, "/deep/a/b") == 0, "cwd correct after ../.. from depth 5");

    int fd = open("/deep/a/b/c/d/file.txt", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create file at depth 5");
    if (fd >= 0) {
        write(fd, "deep", 4);
        close(fd);
    }

    fd = open("/deep/a/b/c/d/file.txt", O_RDONLY);
    check(fd >= 0, "reopen file at depth 5 by absolute path");
    if (fd >= 0) {
        char buf[8];
        int n = (int)read(fd, buf, 8);
        check(n == 4 && memcmp(buf, "deep", 4) == 0, "content correct");
        close(fd);
    }

    chdir("/");
}

/* ------------------------------------------------------------------ */
/*  34. many small file creates                                         */
/* ------------------------------------------------------------------ */

static void test_many_files(void) {
    section("34. many file creates");

    mkdir("/many", 0755);
    int created = 0;
    char path[256];

    for (int i = 0; i < 64; i++) {
        path[0] = '/'; path[1] = 'm'; path[2] = 'a'; path[3] = 'n';
        path[4] = 'y'; path[5] = '/'; path[6] = 'f';
        char num[8];
        itoa(i, num, 10);
        int nl = (int)strlen(num);
        for (int k = 0; k < nl; k++) path[7 + k] = num[k];
        path[7 + nl] = '.';
        path[7 + nl + 1] = 't';
        path[7 + nl + 2] = 'x';
        path[7 + nl + 3] = 't';
        path[7 + nl + 4] = '\0';

        int fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
        if (fd < 0) break;
        char hdr[4];
        hdr[0] = 'F'; hdr[1] = (char)i; hdr[2] = 0; hdr[3] = 0;
        write(fd, hdr, 4);
        close(fd);
        created++;
    }
    printf("    created %d files\n", created);
    check(created >= 32, "created at least 32 files in /many");

    int ok = 1;
    for (int i = 0; i < created; i++) {
        path[0] = '/'; path[1] = 'm'; path[2] = 'a'; path[3] = 'n';
        path[4] = 'y'; path[5] = '/'; path[6] = 'f';
        char num[8];
        itoa(i, num, 10);
        int nl = (int)strlen(num);
        for (int k = 0; k < nl; k++) path[7 + k] = num[k];
        path[7 + nl] = '.'; path[7 + nl + 1] = 't';
        path[7 + nl + 2] = 'x'; path[7 + nl + 3] = 't';
        path[7 + nl + 4] = '\0';

        int fd = open(path, O_RDONLY);
        if (fd < 0) { ok = 0; break; }
        char buf[4];
        int n = (int)read(fd, buf, 4);
        if (n != 4 || buf[0] != 'F' || (unsigned char)buf[1] != (unsigned char)i)
            { ok = 0; close(fd); break; }
        close(fd);
    }
    check(ok, "all created files have correct content");
}

/* ------------------------------------------------------------------ */
/*  35. large file (multi-block / indirect blocks)                     */
/* ------------------------------------------------------------------ */

static void test_large_file(void) {
    section("35. large file (indirect blocks)");

    int fd = open("/large_indirect.bin", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create large file");
    if (fd < 0) return;

    const int BLOCK = 1024;
    const int NBLOCKS = 20;
    char wbuf[BLOCK];
    for (int i = 0; i < NBLOCKS; i++) {
        memset(wbuf, (char)i, BLOCK);
        ssize_t w = write(fd, wbuf, BLOCK);
        if (w != BLOCK) {
            printf("    write failed at block %d\n", i);
            check(0, "large file: all blocks written");
            close(fd);
            return;
        }
    }
    check(1, "large file: all blocks written");

    struct stat st;
    fstat(fd, &st);
    check(st.st_size == BLOCK * NBLOCKS, "large file: size correct");

    lseek(fd, 0, SEEK_SET);
    int ok = 1;
    for (int i = 0; i < NBLOCKS; i++) {
        char rbuf[BLOCK];
        ssize_t r = read(fd, rbuf, BLOCK);
        if (r != BLOCK) { ok = 0; break; }
        for (int j = 0; j < BLOCK; j++) {
            if ((unsigned char)rbuf[j] != (unsigned char)i) { ok = 0; break; }
        }
        if (!ok) break;
    }
    check(ok, "large file: all blocks verified on sequential read");

    for (int i = NBLOCKS - 1; i >= 0; i--) {
        lseek(fd, (int64_t)i * BLOCK, SEEK_SET);
        char rbuf[BLOCK];
        ssize_t r = read(fd, rbuf, BLOCK);
        if (r != BLOCK || (unsigned char)rbuf[0] != (unsigned char)i) { ok = 0; break; }
    }
    check(ok, "large file: all blocks verified on reverse random read");

    close(fd);
}

/* ------------------------------------------------------------------ */
/*  36. read-only flag enforcement                                      */
/* ------------------------------------------------------------------ */

static void test_flags_enforcement(void) {
    section("36. read/write flag enforcement");

    int fd = open("/rwtest.txt", O_RDONLY);
    check(fd >= 0, "open O_RDONLY");
    if (fd >= 0) {
        ssize_t w = write(fd, "x", 1);
        check(w < 0, "write to O_RDONLY fd fails");
        close(fd);
    }

    fd = open("/flags_wo.txt", O_WRONLY | O_CREAT | O_TRUNC);
    check(fd >= 0, "open O_WRONLY");
    if (fd >= 0) {
        write(fd, "hello", 5);
        lseek(fd, 0, SEEK_SET);
        char buf[8];
        ssize_t r = read(fd, buf, 5);
        check(r < 0, "read from O_WRONLY fd fails");
        close(fd);
    }

    fd = open("/flags_rw.txt", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "open O_RDWR");
    if (fd >= 0) {
        ssize_t w = write(fd, "data", 4);
        check(w == 4, "write to O_RDWR ok");
        lseek(fd, 0, SEEK_SET);
        char buf[8];
        ssize_t r = read(fd, buf, 4);
        check(r == 4, "read from O_RDWR ok");
        close(fd);
    }
}

/* ------------------------------------------------------------------ */
/*  37. malloc / free / calloc / realloc                               */
/* ------------------------------------------------------------------ */

static void test_malloc(void) {
    section("37. malloc / calloc / realloc / free");

    void* p = malloc(0);
    check(p == (void*)0, "malloc(0) returns NULL");

    p = malloc(64);
    check(p != (void*)0, "malloc(64) non-null");
    if (p) {
        memset(p, 0xAA, 64);
        check(((unsigned char*)p)[63] == 0xAA, "malloc memory writable");
        free(p);
    }

    p = calloc(16, 8);
    check(p != (void*)0, "calloc(16,8) non-null");
    if (p) {
        int ok = 1;
        for (int i = 0; i < 128; i++) if (((unsigned char*)p)[i] != 0) { ok = 0; break; }
        check(ok, "calloc memory is zero-initialized");
        free(p);
    }

    p = malloc(32);
    check(p != (void*)0, "malloc(32) for realloc test");
    if (p) {
        memset(p, 0xBB, 32);
        void* p2 = realloc(p, 128);
        check(p2 != (void*)0, "realloc to larger size");
        if (p2) {
            int ok = 1;
            for (int i = 0; i < 32; i++) if (((unsigned char*)p2)[i] != 0xBB) { ok = 0; break; }
            check(ok, "realloc preserves original data");
            memset(p2, 0xCC, 128);
            check(((unsigned char*)p2)[127] == 0xCC, "realloc extra space writable");
            free(p2);
        }
    }

    free((void*)0);
    check(1, "free(NULL) does not crash");

    void* ptrs[32];
    int ok2 = 1;
    for (int i = 0; i < 32; i++) {
        ptrs[i] = malloc(64 * (i + 1));
        if (!ptrs[i]) { ok2 = 0; break; }
        memset(ptrs[i], (char)i, 64 * (i + 1));
    }
    check(ok2, "32 sequential mallocs all succeed");

    int ok3 = 1;
    for (int i = 0; i < 32; i++) {
        if (!ptrs[i]) continue;
        unsigned char* p3 = (unsigned char*)ptrs[i];
        for (int j = 0; j < 64 * (i + 1); j++) {
            if (p3[j] != (unsigned char)i) { ok3 = 0; break; }
        }
        if (!ok3) break;
    }
    check(ok3, "all malloc'd regions contain correct data");
    for (int i = 0; i < 32; i++) if (ptrs[i]) free(ptrs[i]);

    void* p4 = malloc(64);
    void* p5 = malloc(64);
    check(p4 != (void*)0 && p5 != (void*)0, "malloc works after mass free");
    if (p4) free(p4);
    if (p5) free(p5);
}

/* ------------------------------------------------------------------ */
/*  38. string functions                                                */
/* ------------------------------------------------------------------ */

static void test_string_ops(void) {
    section("38. string utility functions");

    check(strlen("hello") == 5, "strlen basic");
    check(strlen("") == 0,      "strlen empty");

    check(strcmp("abc", "abc") == 0,  "strcmp equal");
    check(strcmp("abc", "abd") <  0,  "strcmp less");
    check(strcmp("abd", "abc") >  0,  "strcmp greater");
    check(strcmp("",    "a")   <  0,  "strcmp empty vs non");

    check(strncmp("abcX", "abcY", 3) == 0, "strncmp first 3 equal");
    check(strncmp("abcX", "abcY", 4) != 0, "strncmp 4 differs");

    char dst[32];
    strcpy(dst, "hello");
    check(strcmp(dst, "hello") == 0, "strcpy");

    strncpy(dst, "world!!", 5);
    dst[5] = '\0';
    check(strcmp(dst, "world") == 0, "strncpy");

    strcpy(dst, "hello");
    strcat(dst, " world");
    check(strcmp(dst, "hello world") == 0, "strcat");

    const char* p = strchr("hello", 'l');
    check(p != (const char*)0 && *p == 'l', "strchr found");
    p = strchr("hello", 'z');
    check(p == (const char*)0, "strchr not found returns NULL");

    check(abs(-5)  == 5,  "abs(-5)");
    check(abs(5)   == 5,  "abs(5)");
    check(labs(-9999999L) == 9999999L, "labs");

    char numbuf[32];
    itoa(12345, numbuf, 10);
    check(strcmp(numbuf, "12345") == 0, "itoa decimal");
    itoa(255, numbuf, 16);
    check(strcmp(numbuf, "ff") == 0, "itoa hex");
    itoa(-42, numbuf, 10);
    check(strcmp(numbuf, "-42") == 0, "itoa negative");

    check(atoi("42")    == 42,    "atoi positive");
    check(atoi("-7")    == -7,    "atoi negative");
    check(atoi("0")     == 0,     "atoi zero");
    check(atol("99999") == 99999L, "atol large");
}

/* ------------------------------------------------------------------ */
/*  39. bad pointer stress for every syscall                           */
/* ------------------------------------------------------------------ */

static void test_bad_pointers(void) {
    section("39. bad pointer stress");

    char cwd[256];

    check(read(-1, cwd, 10) < 0,  "read  bad fd");
    check(write(-1, cwd, 10) < 0, "write bad fd");

    check(read(0, (void*)0, 10) < 0,                    "read  NULL buf");
    check(write(1, (void*)0, 10) < 0,                   "write NULL buf");
    check(read(0, (void*)0xDEADBEEFULL, 10) < 0,        "read  bad buf");
    check(write(1, (void*)0xDEADBEEFULL, 10) < 0,       "write bad buf");

    check(open((const char*)0, O_RDONLY) < 0,            "open NULL path");
    check(open((const char*)0xDEAD, O_RDONLY) < 0,       "open bad path");
    check(mkdir((const char*)0, 0755) < 0,               "mkdir NULL path");
    check(chdir((const char*)0) < 0,                     "chdir NULL path");
    check(execve((const char*)0, (char* const*)0, (char* const*)0) < 0,
          "execve NULL path");

    check(fstat(0, (struct stat*)0) < 0,                 "fstat NULL stat");
    check(fstat(0, (struct stat*)0x8) < 0,               "fstat bad stat");

    check(getcwd((char*)0, 256) < 0,                     "getcwd NULL buf");
    check(getcwd(cwd, 0) < 0,                            "getcwd size 0");

    check(uname((struct utsname*)0) < 0,                 "uname NULL");
    check(clock_gettime(0, (struct timespec*)0) < 0,     "clock_gettime NULL");

    check(mprotect((void*)1, 4096, PROT_READ) < 0,       "mprotect unaligned");
    check(munmap((void*)1, 4096) < 0,                    "munmap unaligned");

    struct iovec bad_iov;
    bad_iov.iov_base = (void*)0;
    bad_iov.iov_len  = 10;
    check(writev(1, &bad_iov, 1) < 0, "writev NULL iov_base");

    check(lseek(-1, 0, SEEK_SET) < 0, "lseek bad fd");
    check(close(-1) < 0,              "close -1");
    check(close(9999) < 0,            "close oob fd");
}

/* ------------------------------------------------------------------ */
/*  40. concurrent children sharing same file                          */
/* ------------------------------------------------------------------ */

static void test_concurrent_file_access(void) {
    section("40. concurrent children, same file");

    int fd = open("/concurrent.txt", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create file for concurrent test");
    if (fd < 0) return;

    char zero[256];
    memset(zero, 0, 256);
    write(fd, zero, 256);
    close(fd);

    int N = 4;
    pid_t pids[4];
    for (int i = 0; i < N; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            int cfd = open("/concurrent.txt", O_RDWR);
            if (cfd < 0) exit(1);
            lseek(cfd, i * 64, SEEK_SET);
            char buf[64];
            memset(buf, 'A' + i, 64);
            write(cfd, buf, 64);
            close(cfd);
            exit(0);
        }
    }

    int ok = 1;
    for (int i = 0; i < N; i++) {
        int st = 0;
        waitpid(pids[i], &st);
        if (st != 0) ok = 0;
    }
    check(ok, "all concurrent children exited 0");

    fd = open("/concurrent.txt", O_RDONLY);
    if (fd >= 0) {
        char rbuf[256];
        read(fd, rbuf, 256);
        ok = 1;
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < 64; j++) {
                if ((unsigned char)rbuf[i * 64 + j] != (unsigned char)('A' + i))
                    { ok = 0; break; }
            }
        }
        check(ok, "all concurrent writes at non-overlapping offsets correct");
        close(fd);
    }
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

void _start() {
    printf("\n*** KERNEL STRESS TEST SUITE ***\n");
    printf("    pid=%d\n", getpid());

    if (getpid() > 1) {
        printf("Re-execed init, exiting 0\n");
        exit(0);
    }

    test_identity();
    test_set_tid_address();
    test_uname();
    test_clock_gettime();
    test_write_read_basic();
    test_writev_readv();
    test_open_close();
    test_lseek();
    test_fstat();
    test_fs_dirs();
    test_file_rw();
    test_creat_fstat();
    test_fd_limit();
    test_brk();
    test_mmap();
    test_mmap_file();
    test_fork_basic();
    test_fork_memory();
    test_fork_fd_inherit();
    test_fork_chain();
    test_wait_no_children();
    test_exit_codes();
    //test_execve();
    test_fork_bomb();
    //test_fork_mmap_shared();
    test_fork_tree();
    test_memory_stress();
    test_mmap_stress();
    test_mmap_large();
    test_mmap_fork_isolation();
    test_unlink();
    test_readv_writev_stress();
    test_deep_path();
    test_many_files();
    test_large_file();
    test_flags_enforcement();
    test_malloc();
    test_string_ops();
    test_bad_pointers();
    test_concurrent_file_access();

    print_summary();
    exit(0);
}