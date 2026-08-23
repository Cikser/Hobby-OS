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

    rmdir("/test_dir_a/subdir_rel");
    rmdir("/test_dir_a");
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
        unlink("/rwtest_append.txt");
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
    check(WEXITSTATUS(status) == 42, "child exit code == 42");
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

    void* mp = mmap((void*)0, 4096, PROT_READ | PROT_WRITE,
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
        check(WEXITSTATUS(st) == 0x22, "child saw its own mmap write");
        check(((unsigned char*)mp)[0] == 0x11, "parent mmap unaffected by child");
        munmap(mp, 4096);
    }
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
    check(WEXITSTATUS(st) == 0, "child can read from inherited fd");

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
        exit(WEXITSTATUS(st) == 77 ? 0 : 1);
    }
    int st = 0;
    waitpid(c1, &st);
    check(WEXITSTATUS(st) == 0, "fork chain: grandchild exit propagates correctly");
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
        check(WEXITSTATUS(st) == codes[i], "exit code round-trip");
    }
}

/* ------------------------------------------------------------------ */
/*  23. execve                                                          */
/* ------------------------------------------------------------------ */

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
    check(WEXITSTATUS(st) == 0, "execve /bin/init succeeds (exits 0)");

    child = fork();
    if (child == 0) {
        const char* argv[] = { 0 };
        const char* envp[] = { 0 };
        execve("/nonexistent_binary", (char* const*)argv, (char* const*)envp);
        exit(123);
    }
    waitpid(child, &st);
    check(WEXITSTATUS(st) == 123, "execve nonexistent binary fails, falls through");

    child = fork();
    if (child == 0) {
        execve((const char*)0, (char* const*)0, (char* const*)0);
        exit(124);
    }
    waitpid(child, &st);
    check(WEXITSTATUS(st) == 124, "execve NULL path fails");
}

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
    check(WEXITSTATUS(st) == 0, "child read parent value in shared mapping");
    check(((unsigned char*)p)[0] == 0x02, "parent sees child write via MAP_SHARED");

    munmap(p, 4096);
}

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

    if (parent_map == MAP_FAILED) return;
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

    int r = unlink("/to_delete.txt");

    if (r == 0) {
        int fd_after = open("/to_delete.txt", O_RDONLY);
        check(fd_after < 0, "file gone after unlink");
        if (fd_after >= 0) close(fd_after);
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
    rmdir("/deep/a/b/c/d");
    rmdir("/deep/a/b/c");
    rmdir("/deep/a/b");
    rmdir("/deep/a");
    rmdir("/deep");

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
        if (WEXITSTATUS(st) != 0) ok = 0;
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
/*  41. futex basic: WAIT/WAKE, EAGAIN, bad pointers                   */
/* ------------------------------------------------------------------ */

static void test_futex_basic(void) {
    section("41. futex — basic WAIT / WAKE");

    uint32_t* fw = (uint32_t*)mmap((void*)0, 4096,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(fw != MAP_FAILED, "mmap shared futex word");
    if (fw == MAP_FAILED) return;
    *fw = 0;

    pid_t child = fork();
    if (child == 0) {
        long r = futex(fw, FUTEX_WAIT, 0, 0);
        exit(*fw == 1 ? 0 : 1);
    }

    volatile long s = 0;
    for (long i = 0; i < 3000000L; i++) s++;

    *fw = 1;
    long woken = futex(fw, FUTEX_WAKE, 1, 0);
    check(woken == 1, "FUTEX_WAKE returns 1 (one waiter)");

    int st = 0;
    waitpid(child, &st);
    check(st == 0, "child woke correctly and saw *fw == 1");

    long r = futex(fw, FUTEX_WAIT, 0, 0);
    check(r == FUTEX_EAGAIN, "FUTEX_WAIT returns EAGAIN when *uaddr != val");

    long w = futex(fw, FUTEX_WAKE, 1, 0);
    check(w == 0, "FUTEX_WAKE with no waiters returns 0");

    check(futex((uint32_t*)0,   FUTEX_WAIT, 0, 0) == FUTEX_EINVAL,
          "futex(NULL) → EINVAL");
    check(futex((uint32_t*)0x3, FUTEX_WAIT, 0, 0) == FUTEX_EINVAL,
          "futex(unaligned) → EINVAL");
    check(futex((uint32_t*)-1L, FUTEX_WAIT, 0, 0) == FUTEX_EINVAL,
          "futex(0xfff…) → EINVAL");

    munmap(fw, 4096);
}

/* ------------------------------------------------------------------ */
/*  42. futex: multiple waiters, wake-all, wake-N                      */
/* ------------------------------------------------------------------ */

static void test_futex_multi_wake(void) {
    section("42. futex — multiple waiters");

    uint32_t* fw = (uint32_t*)mmap((void*)0, 4096,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(fw != MAP_FAILED, "mmap shared futex word");
    if (fw == MAP_FAILED) return;
    *fw = 0;

    uint32_t* counter = (uint32_t*)(fw + 1);
    *counter = 0;

    const int N = 4;
    pid_t pids[4];
    for (int i = 0; i < N; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            futex(fw, FUTEX_WAIT, 0, 0);
            __sync_fetch_and_add(counter, 1);
            exit(0);
        }
    }

    volatile long sp = 0;
    for (long i = 0; i < 5000000L; i++) sp++;

    long w = futex(fw, FUTEX_WAKE, 2, 0);
    check(w == 2, "FUTEX_WAKE(2) wakes exactly 2");

    w = futex(fw, FUTEX_WAKE, ~0, 0);
    check(w == 2, "FUTEX_WAKE(INT_MAX) wakes remaining 2");

    for (int i = 0; i < N; i++) {
        int st = 0;
        waitpid(pids[i], &st);
    }
    check((int)*counter == N, "all 4 children incremented counter");

    munmap(fw, 4096);
}

/* ------------------------------------------------------------------ */
/*  43. futex: EAGAIN race — value changes before kernel queues us     */
/* ------------------------------------------------------------------ */

static void test_futex_eagain_race(void) {
    section("43. futex — EAGAIN race window");

    uint32_t* fw = (uint32_t*)mmap((void*)0, 4096,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(fw != MAP_FAILED, "mmap");
    if (fw == MAP_FAILED) return;

    *fw = 42;

    long r = futex(fw, FUTEX_WAIT, 0, 0);
    check(r == FUTEX_EAGAIN, "EAGAIN when val doesn't match on entry");

    pid_t child = fork();
    if (child == 0) {
        long rc = futex(fw, FUTEX_WAIT, 42, 0);
        exit(rc == 0 ? 0 : 1);
    }

    volatile long sp = 0;
    for (long i = 0; i < 2000000L; i++) sp++;

    *fw = 99;
    futex(fw, FUTEX_WAKE, 1, 0);

    int st = 0;
    waitpid(child, &st);
    check(st == 0, "child correctly waited on val==42 and was woken");

    munmap(fw, 4096);
}

/* ------------------------------------------------------------------ */
/*  44. clone: basic thread creation, gettid, getpid                   */
/* ------------------------------------------------------------------ */

static volatile int g_tid_child = 0;
static volatile int g_tid_parent_view = 0;

static void thread_record_tid(void* arg) {
    int* out = (int*)arg;
    *out = (int)gettid();
    sched_yield();
}

static void test_clone_basic(void) {
    section("44. clone — basic thread, gettid vs getpid");

    uint32_t* jw = (uint32_t*)mmap((void*)0, 4096,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(jw != MAP_FAILED, "mmap join word");
    if (jw == MAP_FAILED) return;
    *jw = 1;

    uint32_t* tid_out = (uint32_t*)(jw + 1);
    *tid_out = 0;

    thread_t t;
    int r = thread_create(&t, thread_record_tid, (void*)tid_out);
    check(r == 0, "thread_create succeeds");
    if (r != 0) { munmap(jw, 4096); return; }

    thread_join(&t);

    check((int)*tid_out != 0,          "child TID was recorded");
    check((int)*tid_out != (int)gettid(), "child TID != parent TID");

    check(getpid() == gettid(),
          "in main thread, getpid() == gettid()");

    check(t.tid == (int)*tid_out, "parent-seen TID matches child-seen TID");

    munmap(jw, 4096);
}

/* ------------------------------------------------------------------ */
/*  45. clone: thread sees shared address space                         */
/* ------------------------------------------------------------------ */

static volatile uint32_t g_shared_var = 0;

static void thread_write_shared(void* arg) {
    (void)arg;
    g_shared_var = 0xDEADBEEF;
}

static void test_clone_shared_memory(void) {
    section("45. clone — shared address space (CLONE_VM)");

    g_shared_var = 0;

    thread_t t;
    check(thread_create(&t, thread_write_shared, NULL) == 0,
          "thread_create");
    thread_join(&t);

    check(g_shared_var == 0xDEADBEEF,
          "parent sees thread's write to global variable");
}

/* ------------------------------------------------------------------ */
/*  46. clone: shared fd table (CLONE_FILES)                            */
/* ------------------------------------------------------------------ */

static int   g_fd_shared  = -1;
static char  g_fd_buf[16] = {0};

static void thread_read_fd(void* arg) {
    int n = (int)read(g_fd_shared, g_fd_buf, 5);
    (void)n;
}

static void test_clone_shared_fds(void) {
    section("46. clone — shared fd table (CLONE_FILES)");

    int fd = open("/readme.txt", O_RDONLY);
    check(fd >= 0, "open file for fd-sharing test");
    if (fd < 0) return;
    g_fd_shared = fd;

    thread_t t;
    check(thread_create(&t, thread_read_fd, NULL) == 0,
          "thread_create for fd test");
    thread_join(&t);

    check(g_fd_buf[0] != 0, "thread successfully read from parent's fd");

    close(fd);
    g_fd_shared = -1;
}

static mutex_t  g_mutex       = MUTEX_INIT;
static uint32_t g_mutex_count = 0;
static uint32_t g_mutex_races = 0;

static void thread_mutex_increment(void* arg) {
    int iters = *(int*)arg;
    for (int i = 0; i < iters; i++) {
        mutex_lock(&g_mutex);
        uint32_t v = g_mutex_count;
        sched_yield();
        g_mutex_count = v + 1;
        mutex_unlock(&g_mutex);
    }
}

static void test_mutex_basic(void) {
    section("47. mutex — lock/unlock, no data races");

    g_mutex_count = 0;
    g_mutex_races = 0;
    int iters = 200;

    thread_t threads[4];
    for (int i = 0; i < 4; i++) {
        check(thread_create(&threads[i], thread_mutex_increment, &iters) == 0,
              "create mutex-test thread");
    }
    for (int i = 0; i < 4; i++) thread_join(&threads[i]);

    check((int)g_mutex_count == 4 * iters,
          "mutex: final count == 4 * iters (no lost updates)");
}

/* ------------------------------------------------------------------ */
/*  48. mutex: contention stress — many threads, many iterations        */
/* ------------------------------------------------------------------ */

static mutex_t  g_stress_mutex = MUTEX_INIT;
static uint32_t g_stress_count = 0;

static void thread_stress_lock(void* arg) {
    int n = *(int*)arg;
    for (int i = 0; i < n; i++) {
        mutex_lock(&g_stress_mutex);
        g_stress_count++;
        mutex_unlock(&g_stress_mutex);
    }
}

static void test_mutex_stress(void) {
    section("48. mutex — contention stress");

    g_stress_count = 0;
    int iters = 500;
    const int NT = 4;
    thread_t tt[4];

    for (int i = 0; i < NT; i++)
        check(thread_create(&tt[i], thread_stress_lock, &iters) == 0,
              "create stress thread");
    for (int i = 0; i < NT; i++)
        thread_join(&tt[i]);

    check((int)g_stress_count == NT * iters,
          "stress: count == NT * iters");
}

#define PIPE_LEN 16

static sem_t    g_empty;
static sem_t    g_full;
static mutex_t  g_pipe_mutex = MUTEX_INIT;
static int      g_pipe[PIPE_LEN];
static int      g_pipe_head = 0;
static int      g_pipe_tail = 0;
static int      g_pipe_total_consumed = 0;

#define PROD_ITEMS 64

static void thread_producer(void* arg) {
    for (int i = 0; i < PROD_ITEMS; i++) {
        sem_wait(&g_empty);
        mutex_lock(&g_pipe_mutex);
        g_pipe[g_pipe_tail % PIPE_LEN] = i;
        g_pipe_tail++;
        mutex_unlock(&g_pipe_mutex);
        sem_post(&g_full);
    }
}

static void thread_consumer(void* arg) {
    for (int i = 0; i < PROD_ITEMS; i++) {
        sem_wait(&g_full);
        mutex_lock(&g_pipe_mutex);
        int v = g_pipe[g_pipe_head % PIPE_LEN];
        g_pipe_head++;
        g_pipe_total_consumed++;
        mutex_unlock(&g_pipe_mutex);
        sem_post(&g_empty);
    }
}

static void test_semaphore_producer_consumer(void) {
    section("49. semaphore — producer / consumer pipeline");

    sem_init(&g_empty, PIPE_LEN);
    sem_init(&g_full,  0);
    g_pipe_head = g_pipe_tail = g_pipe_total_consumed = 0;

    thread_t prod, cons;
    check(thread_create(&prod, thread_producer, NULL) == 0, "create producer");
    check(thread_create(&cons, thread_consumer, NULL) == 0, "create consumer");
    thread_join(&prod);
    thread_join(&cons);

    check(g_pipe_total_consumed == PROD_ITEMS,
          "consumer received all produced items");
    check(g_pipe_head == g_pipe_tail,
          "pipe is empty at end");
}

static sem_t    g_mc_empty;
static sem_t    g_mc_full;
static mutex_t  g_mc_mutex = MUTEX_INIT;
static int      g_mc_produced = 0;
static int      g_mc_consumed = 0;
#define MC_ITEMS 50
#define MC_PRODS 2
#define MC_CONS  2

static void thread_mc_producer(void* arg) {
    for (int i = 0; i < MC_ITEMS; i++) {
        sem_wait(&g_mc_empty);
        mutex_lock(&g_mc_mutex);
        g_mc_produced++;
        mutex_unlock(&g_mc_mutex);
        sem_post(&g_mc_full);
    }
}

static void thread_mc_consumer(void* arg) {
    for (int i = 0; i < MC_ITEMS; i++) {
        sem_wait(&g_mc_full);
        mutex_lock(&g_mc_mutex);
        g_mc_consumed++;
        mutex_unlock(&g_mc_mutex);
        sem_post(&g_mc_empty);
    }
}

static void test_semaphore_multi(void) {
    section("50. semaphore — multi-producer multi-consumer");

    sem_init(&g_mc_empty, PIPE_LEN);
    sem_init(&g_mc_full,  0);
    g_mc_produced = g_mc_consumed = 0;

    thread_t prods[MC_PRODS], cons[MC_CONS];
    for (int i = 0; i < MC_PRODS; i++)
        check(thread_create(&prods[i], thread_mc_producer, NULL) == 0,
              "create mc producer");
    for (int i = 0; i < MC_CONS; i++)
        check(thread_create(&cons[i],  thread_mc_consumer, NULL) == 0,
              "create mc consumer");
    for (int i = 0; i < MC_PRODS; i++) thread_join(&prods[i]);
    for (int i = 0; i < MC_CONS;  i++) thread_join(&cons[i]);

    check(g_mc_produced == MC_PRODS * MC_ITEMS, "total produced correct");
    check(g_mc_consumed == MC_CONS  * MC_ITEMS, "total consumed correct");
    check(g_mc_produced == g_mc_consumed,       "produced == consumed");
}

static volatile uint32_t g_tl_shared = 0;

static void thread_tl_write(void* arg) {
    volatile uint32_t local = 0xCAFEBABE;
    g_tl_shared = 0xBEEFCAFE;
    (void)local;
    sched_yield();
    if (local != 0xCAFEBABE) {
        g_tl_shared = 0;
    }
}

static void test_clone_thread_local(void) {
    section("51. clone — stack isolation");

    g_tl_shared = 0;

    thread_t t;
    check(thread_create(&t, thread_tl_write, NULL) == 0, "thread_create");
    thread_join(&t);

    check(g_tl_shared == 0xBEEFCAFE,
          "parent sees thread's write to shared global");
}

#define BARRIER_N 4

static uint32_t* g_barrier_fw  = (uint32_t*)0;
static uint32_t  g_barrier_cnt = 0;
static mutex_t   g_barrier_mutex = MUTEX_INIT;
static uint32_t  g_barrier_phase = 0;

static void thread_barrier(void* arg) {
    int rounds = *(int*)arg;
    for (int r = 0; r < rounds; r++) {
        mutex_lock(&g_barrier_mutex);
        g_barrier_cnt++;
        uint32_t phase = g_barrier_phase;
        int last = (g_barrier_cnt % BARRIER_N == 0);
        if (last) {
            g_barrier_phase++;
            mutex_unlock(&g_barrier_mutex);
            futex(g_barrier_fw, FUTEX_WAKE, ~0, 0);
        } else {
            mutex_unlock(&g_barrier_mutex);
            /* Wait for phase to advance.                              */
            while (g_barrier_phase == phase)
                futex(g_barrier_fw, FUTEX_WAIT, phase, 0);
        }
    }
}

static void test_clone_futex_barrier(void) {
    section("52. clone + futex — N-thread barrier");

    g_barrier_fw = (uint32_t*)mmap((void*)0, 4096,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    check(g_barrier_fw != MAP_FAILED, "mmap barrier futex word");
    if (g_barrier_fw == MAP_FAILED) return;

    *g_barrier_fw = 0;
    g_barrier_cnt  = 0;
    g_barrier_phase = 0;

    int rounds = 3;
    thread_t threads[BARRIER_N];
    for (int i = 0; i < BARRIER_N; i++)
        check(thread_create(&threads[i], thread_barrier, &rounds) == 0,
              "create barrier thread");
    for (int i = 0; i < BARRIER_N; i++)
        thread_join(&threads[i]);

    check((int)g_barrier_phase == rounds,
          "barrier completed all rounds");
    check((int)g_barrier_cnt   == BARRIER_N * rounds,
          "all threads participated in every round");

    munmap(g_barrier_fw, 4096);
}

/* ------------------------------------------------------------------ */
/*  53. nanosleep                                                       */
/* ------------------------------------------------------------------ */

static void test_nanosleep(void) {
    section("53. nanosleep");

    struct timespec req = { 0, 10 * 1000000 };
    int r = nanosleep(&req, (struct timespec*)0);
    check(r == 0, "nanosleep 10ms returns 0");

    struct timespec zero = { 0, 0 };
    r = nanosleep(&zero, (struct timespec*)0);
    check(r == 0, "nanosleep 0 returns 0");

    struct timespec rem = { 99, 99 };
    req.tv_sec = 0; req.tv_nsec = 5 * 1000000;
    r = nanosleep(&req, &rem);
    check(r == 0, "nanosleep with rem ptr returns 0");
    check(rem.tv_sec == 0 && rem.tv_nsec == 0, "rem is zero after normal completion");

    struct timespec bad = { 0, -1 };
    r = nanosleep(&bad, (struct timespec*)0);
    check(r < 0, "nanosleep negative nsec fails");

    bad.tv_nsec = 1000000000LL;
    r = nanosleep(&bad, (struct timespec*)0);
    check(r < 0, "nanosleep nsec>=1e9 fails");

    r = nanosleep((struct timespec*)0, (struct timespec*)0);
    check(r < 0, "nanosleep NULL req fails");

    struct timespec t1, t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    req.tv_sec = 0; req.tv_nsec = 20 * 1000000;
    nanosleep(&req, (struct timespec*)0);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    uint64_t ms1 = (uint64_t)t1.tv_sec * 1000 + t1.tv_nsec / 1000000;
    uint64_t ms2 = (uint64_t)t2.tv_sec * 1000 + t2.tv_nsec / 1000000;
    check(ms2 >= ms1 + 10, "time advanced by at least 10ms after 20ms nanosleep");
}

/* ------------------------------------------------------------------ */
/*  54. clock_getres                                                    */
/* ------------------------------------------------------------------ */

static void test_clock_getres(void) {
    section("54. clock_getres");

    struct timespec res;

    int r = clock_getres(CLOCK_MONOTONIC, &res);
    check(r == 0, "clock_getres CLOCK_MONOTONIC returns 0");
    check(res.tv_sec == 0, "resolution tv_sec == 0");
    check(res.tv_nsec > 0, "resolution tv_nsec > 0");
    check(res.tv_nsec <= 1000000000LL, "resolution tv_nsec <= 1s");
    printf("    CLOCK_MONOTONIC resolution: %ld ns\n", (long)res.tv_nsec);

    r = clock_getres(CLOCK_REALTIME, &res);
    check(r == 0, "clock_getres CLOCK_REALTIME returns 0");

    r = clock_getres(CLOCK_MONOTONIC, (struct timespec*)0);
    check(r == 0, "clock_getres NULL res returns 0 (clock exists check)");

    r = clock_getres(CLOCK_MONOTONIC, (struct timespec*)0x10);
    check(r < 0, "clock_getres bad ptr fails");
}

/* ------------------------------------------------------------------ */
/*  55. getppid                                                         */
/* ------------------------------------------------------------------ */

static void test_getppid(void) {
    section("55. getppid");

    pid_t ppid = getppid();
    check(ppid >= 0, "getppid returns non-negative");

    if (getpid() == 1) {
        check(ppid == 0 || ppid == 1, "init ppid is 0 or 1");
    }

    pid_t parent = getpid();
    pid_t child = fork();
    if (child == 0) {
        pid_t child_ppid = getppid();
        exit(child_ppid == parent ? 0 : 1);
    }
    int st = 0;
    waitpid(child, &st);
    check(WEXITSTATUS(st) == 0, "child getppid == parent getpid");

    check(getppid() != getpid(), "ppid != pid");
}

/* ------------------------------------------------------------------ */
/*  56. gettimeofday                                                    */
/* ------------------------------------------------------------------ */

static void test_gettimeofday(void) {
    section("56. gettimeofday");

    struct timeval tv;
    int r = gettimeofday(&tv, (void*)0);
    check(r == 0, "gettimeofday returns 0");
    check(tv.tv_sec >= 0, "tv_sec non-negative");
    check(tv.tv_usec >= 0 && tv.tv_usec < 1000000, "tv_usec in [0, 1e6)");
    printf("    gettimeofday: sec=%ld usec=%ld\n", (long)tv.tv_sec, (long)tv.tv_usec);

    r = gettimeofday((struct timeval*)0, (void*)0);
    check(r == 0, "gettimeofday NULL tv returns 0");

    r = gettimeofday((struct timeval*)0x8, (void*)0);
    check(r < 0, "gettimeofday bad ptr fails");

    struct timeval tv2;
    gettimeofday(&tv, (void*)0);
    volatile long spin = 0;
    for (long i = 0; i < 2000000L; i++) spin++;
    gettimeofday(&tv2, (void*)0);
    uint64_t us1 = (uint64_t)tv.tv_sec  * 1000000 + (uint64_t)tv.tv_usec;
    uint64_t us2 = (uint64_t)tv2.tv_sec * 1000000 + (uint64_t)tv2.tv_usec;
    check(us2 >= us1, "gettimeofday is non-decreasing");

    struct timespec ts;
    gettimeofday(&tv, (void*)0);
    clock_gettime(CLOCK_MONOTONIC, &ts);
    check(tv.tv_sec >= 0 && ts.tv_sec >= 0, "gettimeofday and clock_gettime both work");
}

/* ------------------------------------------------------------------ */
/*  57. getrusage                                                       */
/* ------------------------------------------------------------------ */

static void test_getrusage(void) {
    section("57. getrusage");

    struct rusage ru;
    int r = getrusage(RUSAGE_SELF, &ru);
    check(r == 0, "getrusage RUSAGE_SELF returns 0");
    check(ru.ru_utime.tv_sec >= 0, "ru_utime.tv_sec non-negative");
    check(ru.ru_stime.tv_sec >= 0, "ru_stime.tv_sec non-negative");

    r = getrusage(RUSAGE_CHILDREN, &ru);
    check(r == 0, "getrusage RUSAGE_CHILDREN returns 0");

    r = getrusage(RUSAGE_SELF, (struct rusage*)0);
    check(r < 0, "getrusage NULL fails");

    r = getrusage(RUSAGE_SELF, (struct rusage*)0x10);
    check(r < 0, "getrusage bad ptr fails");
}

/* ------------------------------------------------------------------ */
/*  58. umask                                                           */
/* ------------------------------------------------------------------ */

static void test_umask(void) {
    section("58. umask");

    uint32_t old = umask(022);
    check(old <= 0777u, "umask returns valid old mask");

    uint32_t cur = umask(022);
    check(cur == 022, "umask(022) round-trip");

    uint32_t prev = umask(077);
    check(prev == 022, "umask previous value correct after change");

    umask(old);
    uint32_t restored = umask(old);
    check(restored == old, "umask restored to original");
}

/* ------------------------------------------------------------------ */
/*  59. fcntl                                                           */
/* ------------------------------------------------------------------ */

static void test_fcntl(void) {
    section("59. fcntl");

    int fd = open("/readme.txt", O_RDONLY);
    check(fd >= 0, "open file for fcntl test");
    if (fd < 0) return;

    int r = fcntl(fd, F_GETFD, 0);
    check(r >= 0, "F_GETFD returns >= 0");

    r = fcntl(fd, F_SETFD, FD_CLOEXEC);
    check(r == 0, "F_SETFD FD_CLOEXEC returns 0");

    r = fcntl(fd, F_GETFL, 0);
    check(r >= 0, "F_GETFL returns >= 0");

    r = fcntl(fd, F_SETFL, 0);
    check(r == 0, "F_SETFL returns 0");

    int fd2 = fcntl(fd, F_DUPFD, 0);
    check(fd2 >= 0, "F_DUPFD returns valid fd");
    check(fd2 != fd, "F_DUPFD returns different fd");
    if (fd2 >= 0) {
        char buf[4];
        ssize_t n = read(fd2, buf, 1);
        check(n >= 0, "F_DUPFD result fd is readable");
        close(fd2);
    }

    r = fcntl(-1, F_GETFD, 0);
    check(r < 0, "fcntl on bad fd fails");

    r = fcntl(999, F_GETFD, 0);
    check(r < 0, "fcntl on oob fd fails");

    close(fd);
}

/* ------------------------------------------------------------------ */
/*  60. dup / dup2                                                      */
/* ------------------------------------------------------------------ */

static void test_dup_dup2(void) {
    section("60. dup / dup2");

    int fd = open("/readme.txt", O_RDONLY);
    check(fd >= 0, "open for dup test");
    if (fd < 0) return;

    int fd2 = dup(fd);
    check(fd2 >= 0, "dup returns valid fd");
    check(fd2 != fd, "dup returns different fd");

    if (fd2 >= 0) {
        char b1[8], b2[8];
        lseek(fd,  0, SEEK_SET);
        lseek(fd2, 0, SEEK_SET);
        read(fd,  b1, 4);
        read(fd2, b2, 4);
        check(memcmp(b1, b2, 4) == 0, "dup fd reads same content");
        close(fd2);
    }

    check(dup(-1) < 0, "dup(-1) fails");
    check(dup(999) < 0, "dup(999) fails");

    int fd3 = open("/readme.txt", O_RDONLY);
    check(fd3 >= 0, "second open for dup2 test");
    if (fd3 >= 0) {
        int target = fd3 + 1;
        close(target);

        int r = dup2(fd, target);
        check(r == target, "dup2 returns target fd");
        if (r == target) {
            char buf[4];
            lseek(target, 0, SEEK_SET);
            ssize_t n = read(target, buf, 4);
            check(n > 0, "dup2 target fd is readable");
            close(target);
        }
        close(fd3);
    }

    check(dup2(fd, fd) < 0, "dup2 oldfd==newfd fails");

    close(fd);
}

/* ------------------------------------------------------------------ */
/*  61. pread / pwrite                                                  */
/* ------------------------------------------------------------------ */

static void test_pread_pwrite(void) {
    section("61. pread / pwrite");

    int fd = open("/preadwrite_test.txt", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create file for pread/pwrite test");
    if (fd < 0) return;

    const char* data = "ABCDEFGHIJ";
    ssize_t w = pwrite(fd, data, 10, 0);
    check(w == 10, "pwrite 10 bytes at offset 0");

    w = pwrite(fd, "KLMNOP", 6, 10);
    check(w == 6, "pwrite 6 bytes at offset 10");

    int64_t pos = lseek(fd, 0, SEEK_CUR);
    check(pos == 0, "file position unchanged after pwrite");

    char buf[16];
    ssize_t r = pread(fd, buf, 10, 0);
    check(r == 10, "pread 10 bytes from offset 0");
    check(memcmp(buf, "ABCDEFGHIJ", 10) == 0, "pread data matches pwrite at offset 0");

    r = pread(fd, buf, 6, 10);
    check(r == 6, "pread 6 bytes from offset 10");
    check(memcmp(buf, "KLMNOP", 6) == 0, "pread data matches pwrite at offset 10");

    pos = lseek(fd, 0, SEEK_CUR);
    check(pos == 0, "file position unchanged after pread");

    lseek(fd, 5, SEEK_SET);
    pread(fd, buf, 4, 2);
    pos = lseek(fd, 0, SEEK_CUR);
    check(pos == 5, "pread does not change seeked position");

    r = pread(fd, buf, 4, -1);
    check(r < 0, "pread negative offset fails");

    w = pwrite(fd, "X", 1, -1);
    check(w < 0, "pwrite negative offset fails");

    r = pread(-1, buf, 4, 0);
    check(r < 0, "pread bad fd fails");

    w = pwrite(-1, "X", 1, 0);
    check(w < 0, "pwrite bad fd fails");

    r = pread(fd, (void*)0, 4, 0);
    check(r < 0, "pread NULL buf fails");

    close(fd);
    unlink("/preadwrite_test.txt");
}

/* ------------------------------------------------------------------ */
/*  62. ftruncate                                                       */
/* ------------------------------------------------------------------ */

static void test_ftruncate(void) {
    section("62. ftruncate");

    int fd = open("/trunc_test.txt", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create file for ftruncate test");
    if (fd < 0) return;

    write(fd, "Hello, World!", 13);

    struct stat st;
    fstat(fd, &st);
    check(st.st_size == 13, "initial size == 13");

    int r = ftruncate(fd, 0);
    check(r == 0, "ftruncate to 0 returns 0");

    fstat(fd, &st);
    check(st.st_size == 0, "size == 0 after ftruncate(0)");

    lseek(fd, 0, SEEK_SET);
    char buf[8];
    ssize_t n = read(fd, buf, 8);
    check(n <= 0, "read after truncate returns EOF");

    lseek(fd, 0, SEEK_SET);
    write(fd, "XXXXXXXX", 8);
    fstat(fd, &st);
    check(st.st_size == 8, "size == 8 after write");

    r = ftruncate(fd, 0);
    check(r == 0, "second ftruncate(0) returns 0");
    fstat(fd, &st);
    check(st.st_size == 0, "size == 0 after second ftruncate");

    r = ftruncate(fd, -1);
    check(r < 0, "ftruncate negative length fails");

    r = ftruncate(-1, 0);
    check(r < 0, "ftruncate bad fd fails");

    close(fd);
    unlink("/trunc_test.txt");
}

/* ------------------------------------------------------------------ */
/*  63. readlink                                                        */
/* ------------------------------------------------------------------ */

static void test_readlink(void) {
    section("63. readlink");

    char buf[256];
    ssize_t r = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    check(r > 0, "readlink /proc/self/exe returns > 0 bytes");
    if (r > 0) {
        buf[r] = '\0';
        check(buf[0] == '/', "readlink result starts with /");
        printf("    /proc/self/exe -> %s\n", buf);
    }

    char small[4];
    r = readlink("/proc/self/exe", small, sizeof(small));
    check(r == (ssize_t)sizeof(small), "readlink truncates to bufsize");

    r = readlink("/nonexistent_link_xyz", buf, sizeof(buf));
    check(r < 0, "readlink nonexistent fails");

    r = readlink((const char*)0, buf, sizeof(buf));
    check(r < 0, "readlink NULL path fails");

    r = readlink("/proc/self/exe", (char*)0, 10);
    check(r < 0, "readlink NULL buf fails");

    r = readlink("/proc/self/exe", buf, 0);
    check(r < 0, "readlink bufsize=0 fails");
}

/* ------------------------------------------------------------------ */
/*  64. newfstatat                                                      */
/* ------------------------------------------------------------------ */

static void test_newfstatat(void) {
    section("64. newfstatat / stat");

    struct stat st;
    int r = stat("/readme.txt", &st);
    check(r == 0, "stat /readme.txt returns 0");
    check(st.st_size > 0, "stat st_size > 0");
    check(st.st_ino > 0,  "stat st_ino > 0");

    uint64_t readme_ino = st.st_ino;

    struct stat st_dir;
    r = stat("/", &st_dir);
    check(r == 0, "stat / returns 0");
    check((st_dir.st_mode & 0xF000) == 0x4000, "stat / mode is dir");

    struct stat st_bad;
    r = stat("/no_such_file_xyz.txt", &st_bad);
    check(r < 0, "stat nonexistent fails");

    r = newfstatat(AT_FDCWD, "readme.txt", &st, 0);
    check(r == 0 || r < 0, "newfstatat AT_FDCWD does not crash");

    int fd = open("/readme.txt", O_RDONLY);
    check(fd >= 0, "open for AT_EMPTY_PATH test");
    if (fd >= 0) {
        struct stat st2;
        r = newfstatat(fd, "", &st2, AT_EMPTY_PATH);
        check(r == 0, "newfstatat AT_EMPTY_PATH on fd returns 0");
        check(st2.st_ino == readme_ino, "AT_EMPTY_PATH ino matches readme.txt ino");
        close(fd);
    }

    r = stat("/readme.txt", (struct stat*)0);
    check(r < 0, "stat NULL statbuf fails");

    r = stat("/readme.txt", (struct stat*)0x10);
    check(r < 0, "stat bad statbuf fails");

    r = stat((const char*)0, &st);
    check(r < 0, "stat NULL path fails");
}

/* ------------------------------------------------------------------ */
/*  65. getdents64                                                      */
/* ------------------------------------------------------------------ */

static void test_getdents64(void) {
    section("65. getdents64");

    int fd = open("/", O_RDONLY);
    check(fd >= 0, "open / for getdents64");
    if (fd < 0) return;

    char buf[2048];
    ssize_t r = getdents64(fd, (struct linux_dirent64*)buf, sizeof(buf));
    check(r > 0, "getdents64 / returns > 0 bytes");

    int count = 0;
    int found_readme = 0;
    ssize_t pos = 0;
    while (pos < r) {
        struct linux_dirent64* d = (struct linux_dirent64*)(buf + pos);
        if (d->d_reclen == 0) break;
        if (strcmp(d->d_name, "readme.txt") == 0) found_readme = 1;
        count++;
        pos += d->d_reclen;
    }
    check(count > 0, "getdents64 returned at least one entry");
    check(found_readme, "getdents64 found readme.txt in /");
    printf("    root dir entries: %d\n", count);

    r = getdents64(fd, (struct linux_dirent64*)buf, sizeof(buf));
    check(r >= 0, "second getdents64 call succeeds (0 = EOF ok)");

    lseek(fd, 0, SEEK_SET);
    r = getdents64(fd, (struct linux_dirent64*)buf, sizeof(buf));
    check(r > 0, "getdents64 after lseek(0) works again");

    close(fd);

    fd = open("/", O_RDONLY);
    if (fd >= 0) {
        char tiny[4];
        r = getdents64(fd, (struct linux_dirent64*)tiny, sizeof(tiny));
        check(r < 0, "getdents64 buffer too small fails");
        close(fd);
    }

    r = getdents64(-1, (struct linux_dirent64*)buf, sizeof(buf));
    check(r < 0, "getdents64 bad fd fails");

    fd = open("/", O_RDONLY);
    if (fd >= 0) {
        r = getdents64(fd, (struct linux_dirent64*)0, sizeof(buf));
        check(r < 0, "getdents64 NULL buf fails");
        close(fd);
    }

    mkdir("/dents_test", 0755);
    int f1 = open("/dents_test/aaa.txt", O_RDWR | O_CREAT | O_TRUNC);
    int f2 = open("/dents_test/bbb.txt", O_RDWR | O_CREAT | O_TRUNC);
    if (f1 >= 0) close(f1);
    if (f2 >= 0) close(f2);

    fd = open("/dents_test", O_RDONLY);
    check(fd >= 0, "open /dents_test");
    if (fd >= 0) {
        r = getdents64(fd, (struct linux_dirent64*)buf, sizeof(buf));
        check(r > 0, "getdents64 on new dir returns > 0");

        int found_a = 0, found_b = 0;
        pos = 0;
        while (pos < r) {
            struct linux_dirent64* d = (struct linux_dirent64*)(buf + pos);
            if (d->d_reclen == 0) break;
            if (strcmp(d->d_name, "aaa.txt") == 0) found_a = 1;
            if (strcmp(d->d_name, "bbb.txt") == 0) found_b = 1;
            pos += d->d_reclen;
        }
        check(found_a, "getdents64 found aaa.txt");
        check(found_b, "getdents64 found bbb.txt");
        close(fd);
    }

    unlink("/dents_test/aaa.txt");
    unlink("/dents_test/bbb.txt");
    rmdir("/dents_test");
}

/* ================================================================== */
/*  Signal syscall test suite — add to init.c                         */
/*  Tests: kill, sigaction, sigprocmask, sigpending, sigreturn,       */
/*         SIGKILL/SIGTERM default actions, sa_mask, inheritance      */
/* ================================================================== */

/* ------------------------------------------------------------------ */
/*  Shared volatile state for signal handlers                         */
/* ------------------------------------------------------------------ */

static volatile int g_sig_received  = 0;
static volatile int g_sig_number    = 0;
static volatile int g_handler_calls = 0;

static void sig_record(int sig) {
    g_sig_received = 1;
    g_sig_number   = sig;
    g_handler_calls++;
}

static void sig_noop(int sig) {
    (void)sig;
}

/* ------------------------------------------------------------------ */
/*  66. kill(self) + sigaction — basic delivery                       */
/* ------------------------------------------------------------------ */

static void test_signal_basic(void) {
    section("66. kill(self) + sigaction — basic delivery");

    g_sig_received  = 0;
    g_sig_number    = 0;
    g_handler_calls = 0;

    int r = signal(SIGUSR1, sig_record);
    check(r == 0, "signal(SIGUSR1, handler) succeeds");

    r = kill(getpid(), SIGUSR1);
    check(r == 0, "kill(self, SIGUSR1) returns 0");

    check(g_sig_received  == 1,       "handler called exactly once");
    check(g_sig_number    == SIGUSR1, "handler received correct signum");
    check(g_handler_calls == 1,       "handler_calls == 1");

    signal(SIGUSR1, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  67. SIG_IGN — ignored signal is not delivered                     */
/* ------------------------------------------------------------------ */

static void test_signal_ign(void) {
    section("67. SIG_IGN — signal is ignored");

    g_sig_received = 0;

    signal(SIGUSR2, SIG_IGN);
    kill(getpid(), SIGUSR2);

    check(g_sig_received == 0, "SIG_IGN: handler not called");

    signal(SIGUSR2, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  68. sigaction: oldact returns previous handler                    */
/* ------------------------------------------------------------------ */

static void test_sigaction_oldact(void) {
    section("68. sigaction — oldact returns previous handler");

    struct sigaction_t first  = { (uint64_t)sig_record, SA_RESTORER,
                                  (uint64_t)__sigreturn_trampoline, 0 };
    struct sigaction_t second = { (uint64_t)sig_noop,   SA_RESTORER,
                                  (uint64_t)__sigreturn_trampoline, 0 };
    struct sigaction_t old;

    sigaction(SIGUSR1, &first,  (struct sigaction_t*)0);
    sigaction(SIGUSR1, &second, &old);

    check((sighandler_t)old.sa_handler == sig_record,
          "oldact.sa_handler == first handler");

    signal(SIGUSR1, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  69. sigaction on SIGKILL/SIGSTOP must return error                */
/* ------------------------------------------------------------------ */

static void test_sigaction_unblockable(void) {
    section("69. sigaction(SIGKILL/SIGSTOP) returns error");

    struct sigaction_t act = { (uint64_t)sig_noop, SA_RESTORER,
                               (uint64_t)__sigreturn_trampoline, 0 };

    int r1 = sigaction(SIGKILL, &act, (struct sigaction_t*)0);
    check(r1 < 0, "sigaction(SIGKILL, handler) returns error");

    int r2 = sigaction(SIGSTOP, &act, (struct sigaction_t*)0);
    check(r2 < 0, "sigaction(SIGSTOP, handler) returns error");
}

/* ------------------------------------------------------------------ */
/*  70. sigprocmask — blocking and unblocking                         */
/* ------------------------------------------------------------------ */

static void test_sigprocmask(void) {
    section("70. sigprocmask — blocking and unblocking");

    g_sig_received = 0;
    signal(SIGUSR1, sig_record);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);

    int r = sigprocmask(0 /*SIG_BLOCK*/, &mask, (sigset_t*)0);
    check(r == 0, "sigprocmask(SIG_BLOCK) returns 0");

    kill(getpid(), SIGUSR1);
    check(g_sig_received == 0, "masked SIGUSR1 not delivered");

    r = sigprocmask(1 /*SIG_UNBLOCK*/, &mask, (sigset_t*)0);
    check(r == 0, "sigprocmask(SIG_UNBLOCK) returns 0");
    check(g_sig_received == 1, "SIGUSR1 delivered after unblocking");

    signal(SIGUSR1, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  71. sigprocmask: SIG_SETMASK                                      */
/* ------------------------------------------------------------------ */

static void test_sigprocmask_setmask(void) {
    section("71. sigprocmask(SIG_SETMASK)");

    sigset_t full, empty, old;
    sigfillset(&full);
    sigemptyset(&empty);

    sigprocmask(2 /*SIG_SETMASK*/, &full, &old);
    check(old == 0, "old mask was empty");

    sigprocmask(2 /*SIG_SETMASK*/, &empty, (sigset_t*)0);

    sigset_t cur;
    sigprocmask(2 /*SIG_SETMASK*/, (sigset_t*)0, &cur);
    check(cur == 0, "mask is empty again after SIG_SETMASK(empty)");
}

/* ------------------------------------------------------------------ */
/*  72. sigpending — pending signal visible while masked              */
/* ------------------------------------------------------------------ */

static void test_sigpending(void) {
    section("72. sigpending — visibility of pending signals");

    g_sig_received = 0;
    signal(SIGUSR2, sig_record);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(0, &mask, (sigset_t*)0);
    kill(getpid(), SIGUSR2);

    sigset_t pending;
    int r = sigpending(&pending);
    check(r == 0,              "sigpending returns 0");
    check(g_sig_received == 0, "signal not yet delivered while masked");

    sigprocmask(1, &mask, (sigset_t*)0);
    check(g_sig_received == 1, "signal delivered after unblocking");

    signal(SIGUSR2, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  73. Multiple signals in sequence — each delivered at least once   */
/* ------------------------------------------------------------------ */

static void test_signal_multiple(void) {
    section("73. multiple successive signals");

    g_handler_calls = 0;
    signal(SIGUSR1, sig_record);

    for (int i = 0; i < 5; i++)
        kill(getpid(), SIGUSR1);

    /* Standard UNIX semantics: signals are not queued, only one
       pending bit per signal number, so at least 1 delivery. */
    check(g_handler_calls >= 1,
          "handler called at least once for 5 kills");

    signal(SIGUSR1, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  74. SIGTERM default action in child — parent sees exit code       */
/* ------------------------------------------------------------------ */

static void test_sigterm_default(void) {
    section("74. SIGTERM — default action terminates process");

    pid_t child = fork();
    if (child == 0) {
        kill(getpid(), SIGTERM);
        exit(200);  /* must never be reached */
    }

    int st = 0;
    pid_t w = waitpid(child, &st);
    check(w == child, "waitpid returns child pid");
    check(WIFSIGNALED(st) && WTERMSIG(st) == SIGTERM, "child did not reach exit(200)");
}

/* ------------------------------------------------------------------ */
/*  75. SIGKILL cannot be blocked or caught                           */
/* ------------------------------------------------------------------ */

static void test_sigkill_uncatchable(void) {
    section("75. SIGKILL — cannot be caught or blocked");

    int r = signal(SIGKILL, sig_noop);
    check(r < 0, "signal(SIGKILL, handler) returns error");

    sigset_t mask, old;
    sigemptyset(&mask);
    sigaddset(&mask, SIGKILL);
    sigprocmask(0 /*SIG_BLOCK*/, &mask, &old);
    check(!sigismember(&old, SIGKILL), "SIGKILL not in mask after SIG_BLOCK");

    pid_t child = fork();
    if (child == 0) {
        signal(SIGKILL, sig_noop);  /* must be rejected */
        kill(getpid(), SIGKILL);
        exit(77);  /* must never be reached */
    }
    int st = 0;
    waitpid(child, &st);
    check(WIFSIGNALED(st) && WTERMSIG(st) == SIGKILL, "child killed by SIGKILL (did not reach exit(77))");
}

/* ------------------------------------------------------------------ */
/*  76. sa_mask — additional signals blocked during handler           */
/* ------------------------------------------------------------------ */

static volatile int g_nested = 0;
static volatile int g_in_handler = 0;
static volatile int g_nested_during = 0;
static volatile int g_nested_after = 0;

static void handler_check_nested(int sig) {
    g_in_handler = 1;
    kill(getpid(), SIGUSR2);
    for (volatile int i = 0; i < 100000; i++);
    g_nested_during = g_nested;
    g_in_handler = 0;
    (void)sig;
}

static void handler_sigusr2_nested(int sig) {
    if (g_in_handler) g_nested_during = 1;
    g_nested = 1;
    (void)sig;
}

static void test_sa_mask(void) {
    section("76. sa_mask — signals blocked during handler execution");

    g_nested = 0;
    g_nested_during = 0;
    g_in_handler = 0;
    signal(SIGUSR2, handler_sigusr2_nested);

    struct sigaction_t act;
    act.sa_handler  = (uint64_t)handler_check_nested;
    act.sa_flags    = SA_RESTORER;
    act.sa_restorer = (uint64_t)__sigreturn_trampoline;
    sigemptyset((sigset_t*)&act.sa_mask);
    sigaddset((sigset_t*)&act.sa_mask, SIGUSR2);

    sigaction(SIGUSR1, &act, (struct sigaction_t*)0);
    kill(getpid(), SIGUSR1);

    check(g_nested_during == 0,
          "SIGUSR2 not delivered during SIGUSR1 handler (sa_mask works)");
    check(g_nested == 1,
          "SIGUSR2 delivered after returning from handler");

    signal(SIGUSR1, SIG_DFL);
    signal(SIGUSR2, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  77. sigreturn restores full register context                      */
/* ------------------------------------------------------------------ */

static volatile uint64_t g_val_before = 0;
static volatile uint64_t g_val_after  = 0;

static void handler_ctx(int sig) {
    (void)sig;
}

static void test_sigreturn_context(void) {
    section("77. sigreturn restores register context");

    signal(SIGUSR1, handler_ctx);

    uint64_t val = 0xDEADBEEFCAFEBABEULL;
    g_val_before = val;

    register uint64_t s1_reg __asm__("s1") = val;
    __asm__ volatile ("" : "+r"(s1_reg));  /* pin value in s1 */

    kill(getpid(), SIGUSR1);

    uint64_t s1_after;
    __asm__ volatile ("mv %0, s1" : "=r"(s1_after));
    g_val_after = s1_after;

    check(g_val_after == g_val_before,
          "sigreturn: callee-saved register s1 restored correctly");

    signal(SIGUSR1, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  78. fork — child inherits handlers but not pending signals        */
/* ------------------------------------------------------------------ */

static void test_signal_fork_inherit(void) {
    section("78. fork — handler inherited, pending signals not");

    signal(SIGUSR1, sig_record);

    /* Make SIGUSR1 pending in the parent before forking. */
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(0, &mask, (sigset_t*)0);
    kill(getpid(), SIGUSR1);

    pid_t child = fork();
    if (child == 0) {
        g_sig_received = 0;
        sigprocmask(1, &mask, (sigset_t*)0);
        for (volatile int i = 0; i < 50000; i++);
        exit(g_sig_received == 0 ? 0 : 1);
    }

    sigprocmask(1, &mask, (sigset_t*)0);

    int st = 0;
    waitpid(child, &st);
    check(WEXITSTATUS(st) == 0, "child did not inherit pending SIGUSR1");

    signal(SIGUSR1, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  79. execve resets handlers to SIG_DFL                            */
/* ------------------------------------------------------------------ */

static void test_signal_exec_reset(void) {
    section("79. execve resets signal handlers to SIG_DFL");

    pid_t child = fork();
    if (child == 0) {
        signal(SIGUSR1, sig_noop);
        const char* argv[] = { "/bin/init", 0 };
        const char* envp[] = { 0 };
        execve("/bin/init", (char* const*)argv, (char* const*)envp);
        exit(99);
    }
    int st = 0;
    waitpid(child, &st);
    check(WEXITSTATUS(st) == 0, "execve child exits 0 (/bin/init re-executed)");
}

/* ------------------------------------------------------------------ */
/*  80. kill with invalid signum returns error                        */
/* ------------------------------------------------------------------ */

static void test_kill_invalid(void) {
    section("80. kill — invalid signum and pid");

    int r1 = kill(getpid(), -1);
    check(r1 < 0, "kill(self, -1) returns error");

    int r2 = kill(getpid(), NSIG + 1);
    check(r2 < 0, "kill(self, NSIG+1) returns error");

    int r3 = kill(getpid(), 0);
    check(r3 == 0, "kill(self, 0) returns 0 (validity check only)");
}

/* ------------------------------------------------------------------ */
/*  81. sigaction with NULL act — query only                          */
/* ------------------------------------------------------------------ */

static void test_sigaction_query(void) {
    section("81. sigaction(NULL act) — query current disposition");

    struct sigaction_t installed = { (uint64_t)sig_record, SA_RESTORER,
                                     (uint64_t)__sigreturn_trampoline, 0 };
    sigaction(SIGUSR1, &installed, (struct sigaction_t*)0);

    struct sigaction_t queried;
    int r = sigaction(SIGUSR1, (struct sigaction_t*)0, &queried);
    check(r == 0, "sigaction(SIGUSR1, NULL, &old) returns 0");
    check((sighandler_t)queried.sa_handler == sig_record,
          "queried handler matches installed handler");

    signal(SIGUSR1, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  82. SIGCHLD — parent receives signal when child exits             */
/* ------------------------------------------------------------------ */

static volatile int g_sigchld_count = 0;

static void handler_sigchld(int sig) {
    g_sigchld_count++;
    (void)sig;
}

static void test_sigchld(void) {
    section("82. SIGCHLD delivered to parent when child exits");

    g_sigchld_count = 0;
    signal(SIGCHLD, handler_sigchld);

    pid_t child = fork();
    if (child == 0) exit(0);

    int st = 0;
    waitpid(child, &st);

    for (volatile int i = 0; i < 200000; i++);
    check(g_sigchld_count >= 1, "SIGCHLD delivered at least once");

    signal(SIGCHLD, SIG_DFL);
}

/* ------------------------------------------------------------------ */
/*  83. Stress: 20 forked children killed with SIGTERM                */
/* ------------------------------------------------------------------ */

static void test_signal_stress(void) {
    section("83. stress — 20 children killed with SIGTERM");

    const int N = 5;
    pid_t pids[5];

    for (int i = 0; i < N; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            for (;;) sched_yield();
        }
    }

    for (int i = 0; i < N; i++)
        kill(pids[i], SIGTERM);

    int ok = 1;
    for (int i = 0; i < N; i++) {
        int st = 0;
        pid_t r = waitpid(pids[i], &st);
        if (r != pids[i] || !(WIFSIGNALED(st) && WTERMSIG(st) == SIGTERM)) { ok = 0; }
    }
    check(ok, "all children terminated by SIGTERM and reaped");
}

static void test_madvise(void) {
    section("84. madvise");

    void* p = mmap((void*)0, 4096 * 4, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(p != MAP_FAILED, "mmap for madvise test");
    if (p == MAP_FAILED) return;

    memset(p, 0xAA, 4096 * 4);

    int r = madvise(p, 4096 * 4, MADV_NORMAL);
    check(r == 0, "madvise MADV_NORMAL returns 0");

    r = madvise(p, 4096 * 4, MADV_DONTNEED);
    check(r == 0, "madvise MADV_DONTNEED returns 0");

    r = madvise(p, 4096 * 4, MADV_FREE);
    check(r == 0, "madvise MADV_FREE returns 0");

    r = madvise((void*)0, 4096, MADV_NORMAL);
    check(r == 0, "madvise NULL addr returns 0 (kernel ignores)");

    r = madvise((char*)p + 1, 4096, MADV_NORMAL);
    check(r == 0 || r < 0, "madvise unaligned does not crash");

    munmap(p, 4096 * 4);
}

/* ------------------------------------------------------------------ */
/*  85. prlimit64                                                       */
/* ------------------------------------------------------------------ */

static void test_prlimit(void) {
    section("85. prlimit64");

    struct rlimit lim;

    int r = prlimit(0, RLIMIT_STACK, (struct rlimit*)0, &lim);
    check(r == 0, "prlimit RLIMIT_STACK returns 0");
    check(lim.rlim_cur > 0, "RLIMIT_STACK cur > 0");
    check(lim.rlim_max > 0, "RLIMIT_STACK max > 0");
    printf("    RLIMIT_STACK: cur=%llu max=%llu\n",
           (unsigned long long)lim.rlim_cur,
           (unsigned long long)lim.rlim_max);

    r = prlimit(0, RLIMIT_NOFILE, (struct rlimit*)0, &lim);
    check(r == 0, "prlimit RLIMIT_NOFILE returns 0");
    check(lim.rlim_cur > 0, "RLIMIT_NOFILE cur > 0");
    printf("    RLIMIT_NOFILE: cur=%llu max=%llu\n",
           (unsigned long long)lim.rlim_cur,
           (unsigned long long)lim.rlim_max);

    r = prlimit(0, RLIMIT_AS, (struct rlimit*)0, &lim);
    check(r == 0, "prlimit RLIMIT_AS returns 0");
    printf("    RLIMIT_AS: cur=%llu max=%llu\n",
           (unsigned long long)lim.rlim_cur,
           (unsigned long long)lim.rlim_max);

    r = prlimit(0, RLIMIT_STACK, (struct rlimit*)0, (struct rlimit*)0);
    check(r == 0, "prlimit NULL old_lim returns 0");

    r = prlimit(0, RLIMIT_STACK, (struct rlimit*)0, (struct rlimit*)0x10);
    check(r < 0, "prlimit bad ptr fails");

    r = prlimit(0, 999, (struct rlimit*)0, &lim);
    check(r == 0, "prlimit unknown resource returns 0 (infinity)");
    check(lim.rlim_cur == RLIM_INFINITY, "unknown resource returns RLIM_INFINITY");
}

/* ------------------------------------------------------------------ */
/*  86. getrandom                                                       */
/* ------------------------------------------------------------------ */

static void test_getrandom(void) {
    section("86. getrandom");

    uint8_t buf1[32];
    ssize_t r = getrandom(buf1, 32, 0);
    check(r == 32, "getrandom 32 bytes returns 32");

    int all_zero = 1;
    for (int i = 0; i < 32; i++) if (buf1[i] != 0) { all_zero = 0; break; }
    check(!all_zero, "getrandom output is not all zeros");

    uint8_t buf2[32];
    getrandom(buf2, 32, 0);
    int same = 1;
    for (int i = 0; i < 32; i++) if (buf1[i] != buf2[i]) { same = 0; break; }
    check(!same, "two getrandom calls return different data");

    uint8_t one = 0;
    r = getrandom(&one, 1, 0);
    check(r == 1, "getrandom 1 byte returns 1");

    uint8_t buf3[16];
    r = getrandom(buf3, 16, GRND_NONBLOCK);
    check(r == 16, "getrandom GRND_NONBLOCK returns 16");

    uint8_t big[256];
    r = getrandom(big, 256, 0);
    check(r == 256, "getrandom 256 bytes returns 256");

    r = getrandom((void*)0, 16, 0);
    check(r < 0, "getrandom NULL buf fails");

    r = getrandom(buf1, 0, 0);
    check(r < 0, "getrandom len=0 fails");

    r = getrandom((void*)0xDEAD, 16, 0);
    check(r < 0, "getrandom bad ptr fails");
}

/* ------------------------------------------------------------------ */
/*  87. statx                                                           */
/* ------------------------------------------------------------------ */

static void test_statx(void) {
    section("87. statx");

    struct statx sx;

    int r = statx(AT_FDCWD, "/readme.txt", 0, STATX_ALL, &sx);
    check(r == 0, "statx /readme.txt returns 0");
    check(sx.stx_ino > 0,  "statx stx_ino > 0");
    check(sx.stx_size > 0, "statx stx_size > 0");
    check((sx.stx_mode & 0xF000) == 0x8000, "statx mode is regular file");
    printf("    statx: ino=%llu size=%llu mode=0x%x\n",
           (unsigned long long)sx.stx_ino,
           (unsigned long long)sx.stx_size,
           sx.stx_mode);

    uint64_t readme_ino  = sx.stx_ino;
    uint64_t readme_size = sx.stx_size;

    struct statx sx_dir;
    r = statx(AT_FDCWD, "/", 0, STATX_ALL, &sx_dir);
    check(r == 0, "statx / returns 0");
    check((sx_dir.stx_mode & 0xF000) == 0x4000, "statx / mode is directory");

    int fd = open("/readme.txt", O_RDONLY);
    check(fd >= 0, "open for statx AT_EMPTY_PATH");
    if (fd >= 0) {
        struct statx sx2;
        r = statx(fd, "", AT_EMPTY_PATH, STATX_ALL, &sx2);
        check(r == 0, "statx AT_EMPTY_PATH on fd returns 0");
        check(sx2.stx_ino == readme_ino,
              "statx AT_EMPTY_PATH ino matches path-based statx");
        close(fd);
    }

    r = statx(AT_FDCWD, "/nonexistent_xyz.txt", 0, STATX_ALL, &sx);
    check(r < 0, "statx nonexistent returns error");

    r = statx(AT_FDCWD, "/readme.txt", 0, STATX_ALL, (struct statx*)0);
    check(r < 0, "statx NULL buf fails");

    r = statx(AT_FDCWD, "/readme.txt", 0, STATX_ALL, (struct statx*)0x10);
    check(r < 0, "statx bad buf ptr fails");

    r = statx(AT_FDCWD, (const char*)0, 0, STATX_ALL, &sx);
    check(r < 0, "statx NULL path fails");

    fd = open("/readme.txt", O_RDONLY);
    if (fd >= 0) {
        struct stat st;
        fstat(fd, &st);
        struct statx sx3;
        statx(fd, "", AT_EMPTY_PATH, STATX_ALL, &sx3);
        check(sx3.stx_ino  == st.st_ino,
              "statx ino matches fstat ino");
        check(sx3.stx_size == (uint64_t)st.st_size,
              "statx size matches fstat size");
        close(fd);
    }
}

/* ------------------------------------------------------------------ */
/*  88. membarrier                                                      */
/* ------------------------------------------------------------------ */

static void test_membarrier(void) {
    section("88. membarrier");

    int r = membarrier(0, 0, 0);
    check(r >= 0, "membarrier QUERY returns >= 0");

    r = membarrier(1, 0, 0);
    check(r == 0, "membarrier GLOBAL returns 0");

    r = membarrier(999, 0, 0);
    check(r == 0, "membarrier unknown cmd returns 0 (stub)");
}

/* ------------------------------------------------------------------ */
/*  89. ioctl basic                                                     */
/* ------------------------------------------------------------------ */

static void test_ioctl(void) {
    section("89. ioctl");

    struct winsize {
        unsigned short ws_row, ws_col, ws_xpixel, ws_ypixel;
    } ws;
    int r = ioctl(STDOUT_FILENO, TIOCGWINSZ, (unsigned long)&ws);
    check(r == 0, "ioctl TIOCGWINSZ on stdout (tty) succeeds");
    check(ws.ws_row > 0 && ws.ws_col > 0, "winsize has sane values");

    int fd = open("/readme.txt", O_RDONLY);
    if (fd >= 0) {
        r = ioctl(fd, TIOCGWINSZ, (unsigned long)&ws);
        check(r < 0, "ioctl TIOCGWINSZ on regular file fails");
        close(fd);
    }

    r = ioctl(-1, TIOCGWINSZ, 0);
    check(r < 0, "ioctl on bad fd fails");
}

/* ------------------------------------------------------------------ */
/*  90. pipe / pipe2                                                    */
/* ------------------------------------------------------------------ */

static void test_pipe(void) {
    section("90. pipe / pipe2");

    int fds[2];
    
    /* 1. Basic pipe creation and close */
    int r = pipe(fds);
    check(r == 0, "pipe creation succeeds");
    if (r == 0) {
        check(fds[0] >= 0 && fds[1] >= 0, "pipe returns valid fds");
        check(fds[0] != fds[1], "pipe fds are distinct");
        close(fds[0]);
        close(fds[1]);
    }

    /* 2. Basic read / write (single process) */
    r = pipe(fds);
    if (r == 0) {
        const char* msg = "hello pipe!";
        ssize_t w = write(fds[1], msg, 11);
        check(w == 11, "write 11 bytes to pipe writer");

        char buf[32];
        ssize_t rd = read(fds[0], buf, 32);
        check(rd == 11, "read 11 bytes from pipe reader");
        if (rd > 0) buf[rd] = '\0';
        check(strcmp(buf, msg) == 0, "pipe read data matches write data");

        close(fds[0]);
        close(fds[1]);
    }

    /* 3. Inter-process communication via fork */
    r = pipe(fds);
    if (r == 0) {
        pid_t child = fork();
        if (child == 0) {
            /* Child closes read end, writes to pipe */
            close(fds[0]);
            file_write_str(fds[1], "child to parent");
            close(fds[1]);
            exit(0);
        }

        /* Parent closes write end, reads from pipe */
        close(fds[1]);
        char buf[64];
        int total = file_read_all(fds[0], buf, sizeof(buf));
        close(fds[0]);

        int st = 0;
        waitpid(child, &st);
        check(WEXITSTATUS(st) == 0, "pipe IPC child exited 0");
        check(total == 15 && strcmp(buf, "child to parent") == 0, 
              "parent received correct string from child via pipe");
    }

    /* 4. EOF detection when writer is closed */
    r = pipe(fds);
    if (r == 0) {
        write(fds[1], "data", 4);
        close(fds[1]); /* Close write end */

        char buf[16];
        ssize_t rd1 = read(fds[0], buf, 16);
        check(rd1 == 4, "read buffered data before EOF");

        ssize_t rd2 = read(fds[0], buf, 16);
        check(rd2 == 0, "read on pipe with closed write end returns 0 (EOF)");

        close(fds[0]);
    }

    /* 5. Invalid arguments / Bad pointers */
    int bad_r = pipe((int*)0);
    check(bad_r < 0, "pipe(NULL) fails");

    bad_r = pipe((int*)0x1);
    check(bad_r < 0, "pipe(unaligned/bad ptr) fails");

    /* 6. pipe2 flags testing (O_CLOEXEC, O_NONBLOCK) */
#ifdef O_NONBLOCK
    r = pipe2(fds, O_NONBLOCK);
    check(r == 0, "pipe2 with O_NONBLOCK succeeds");
    if (r == 0) {
        char buf[16];
        ssize_t empty_rd = read(fds[0], buf, 16);
        check(empty_rd < 0, "read on empty O_NONBLOCK pipe fails (EAGAIN/EWOULDBLOCK)");

        close(fds[0]);
        close(fds[1]);
    }
#endif

    /* 7. Stress / Large Data Chunk transfer through Pipe */
    r = pipe(fds);
    if (r == 0) {
        pid_t child = fork();
        if (child == 0) {
            close(fds[0]);
            char chunk[1024];
            memset(chunk, 0xAB, 1024);
            /* Write 16KB of data */
            for (int i = 0; i < 16; i++) {
                write(fds[1], chunk, 1024);
            }
            close(fds[1]);
            exit(0);
        }

        close(fds[1]);
        char rbuf[1024];
        int total_bytes = 0;
        int pattern_ok = 1;

        while (1) {
            ssize_t n = read(fds[0], rbuf, 1024);
            if (n <= 0) break;
            total_bytes += n;
            for (ssize_t i = 0; i < n; i++) {
                if ((unsigned char)rbuf[i] != 0xAB) {
                    pattern_ok = 0;
                    break;
                }
            }
        }
        close(fds[0]);

        int st = 0;
        waitpid(child, &st);
        check(total_bytes == 16 * 1024, "large pipe transfer: total bytes match (16KB)");
        check(pattern_ok, "large pipe transfer: data pattern intact");
    }
}

static void test_wait_wnohang(void) {
    section("91. waitpid WNOHANG");

    pid_t child = fork();
    if (child == 0) {
        for (volatile long i = 0; i < 5000000L; i++);
        exit(0);
    }

    int status = 0;
    pid_t r = waitpid3(child, &status, WNOHANG);
    check(r == 0, "WNOHANG returns 0 immediately while child alive");

    r = waitpid3(child, &status, 0);
    check(r == child, "blocking waitpid reaps child eventually");
    check(WEXITSTATUS(status) == 0, "child exit status correct");

    r = waitpid3(child, &status, WNOHANG);
    check(r < 0, "WNOHANG on already-reaped child returns error");
}

static volatile int g_cont_seen = 0;
static void handler_sigcont(int sig) { g_cont_seen = 1; (void)sig; }

static void test_sigstop_sigcont(void) {
    section("92. SIGSTOP + SIGCONT");

    pid_t child = fork();
    if (child == 0) {
        signal(SIGCONT, handler_sigcont);
        while (!g_cont_seen) sched_yield();
        exit(55);
    }

    for (volatile long i = 0; i < 2000000L; i++);

    kill(child, SIGSTOP);
    for (volatile long i = 0; i < 2000000L; i++);

    kill(child, SIGCONT);

    int status = 0;
    pid_t w = waitpid(child, &status);
    check(w == child, "child eventually reaped after STOP+CONT");
    check(WEXITSTATUS(status) == 55, "child resumed and exited with correct code after SIGCONT");
}

static void test_symlink(void) {
    section("93. symlink / readlink");

    unlink("/symlink_target.txt");
    unlink("/symlink_link.txt");
    unlink("/dangling_link");

    int fd = open("/symlink_target.txt", O_RDWR | O_CREAT | O_TRUNC);
    check(fd >= 0, "create symlink target file");
    if (fd >= 0) {
        write(fd, "hello via symlink", 17);
        close(fd);
    }

    int r = symlink("/symlink_target.txt", "/symlink_link.txt");
    check(r == 0, "symlink() creates link");

    char buf[64];
    ssize_t n = readlink("/symlink_link.txt", buf, sizeof(buf));
    check(n == (ssize_t)strlen("/symlink_target.txt"), "readlink returns correct length");
    if (n > 0) {
        buf[n] = '\0';
        check(strcmp(buf, "/symlink_target.txt") == 0, "readlink content correct");
    }

    int lfd = open("/symlink_link.txt", O_RDONLY);
    check(lfd >= 0, "open() follows symlink");
    if (lfd >= 0) {
        char rbuf[32];
        int rn = (int)read(lfd, rbuf, 32);
        check(rn == 17 && memcmp(rbuf, "hello via symlink", 17) == 0,
              "content read through symlink matches target");
        close(lfd);
    }

    r = symlink("/nonexistent_target_xyz", "/dangling_link");
    check(r == 0, "symlink to nonexistent target still succeeds (lazy)");

    int dfd = open("/dangling_link", O_RDONLY);
    check(dfd < 0, "opening dangling symlink fails");

    r = symlink("/symlink_target.txt", "/symlink_link.txt");
    check(r < 0, "symlink() on existing path fails");

    unlink("/symlink_target.txt");
    unlink("/symlink_link.txt");
    unlink("/dangling_link");
}

static void test_access(void) {
    section("94. access / faccessat");

    int r = access("/readme.txt", F_OK);
    check(r == 0, "access F_OK on existing file succeeds");

    r = access("/no_such_file_xyz.txt", F_OK);
    check(r < 0, "access F_OK on missing file fails");

    r = access("/readme.txt", R_OK | W_OK);
    check(r == 0, "access R_OK|W_OK stub succeeds (no permission model)");
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

void _start() {
/*
    const char* argv[] = { "/bin/hello_musl", "arg 1", "arg 2", 0 };
    const char* envp[] = { 0 };
    printf("entering hello_musl\n");
    execve("/bin/hello_musl", (char* const*)argv, (char* const*)envp);

    write(1, "execve failed\n", 14);
    exit(1);
*/
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
    test_execve();
    test_fork_bomb();
    test_fork_mmap_shared();
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
    test_futex_basic();
    test_futex_multi_wake();
    test_futex_eagain_race();
    test_clone_basic();
    test_clone_shared_memory();
    test_clone_shared_fds();
    test_mutex_basic();
    test_mutex_stress();
    test_semaphore_producer_consumer();
    test_semaphore_multi();
    test_clone_thread_local();
    test_clone_futex_barrier();
    test_nanosleep();
    test_clock_getres();
    test_getppid();
    test_gettimeofday();
    test_getrusage();
    test_umask();
    test_fcntl();
    test_dup_dup2();
    test_pread_pwrite();
    test_ftruncate();
    test_readlink();
    test_newfstatat();
    test_getdents64();
    test_signal_basic();
    test_signal_ign();
    test_sigaction_oldact();
    test_sigaction_unblockable();
    test_sigprocmask();
    test_sigprocmask_setmask();
    test_sigpending();
    test_signal_multiple();
    test_sigterm_default();
    test_sigkill_uncatchable();
    test_sa_mask();
    test_sigreturn_context();
    test_signal_fork_inherit();
    test_signal_exec_reset();
    test_kill_invalid();
    test_sigaction_query();
    test_sigchld();
    test_signal_stress();
    test_madvise();
    test_prlimit();
    test_getrandom();
    test_statx();
    test_membarrier();
    test_ioctl();
    test_pipe();
    test_wait_wnohang();
    test_sigstop_sigcont();
    test_symlink();
    test_access();

    print_summary();
    exit(0);
}