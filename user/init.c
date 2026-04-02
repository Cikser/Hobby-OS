#include "syscall.h"

#define PROT_READ  0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20


void print_header(const char* title) {
    printf("\n========================================\n");
    printf("%s\n", title);
    printf("========================================\n");
}

void test_basic_fs() {
    print_header("TEST: Basic VFS & FSTAT");

    char cwd[256];
    if (getcwd(cwd, 256) == 0) {
        printf("CWD: %s\n", cwd);
    } else {
        printf("FAILED: getcwd\n");
    }

    if (mkdir("/testdir", 0755) == 0) {
        printf("MKDIR: /testdir created\n");
    } else {
        printf("FAILED: mkdir /testdir\n");
    }

    if (chdir("/testdir") == 0) {
        printf("CHDIR: Moved to /testdir\n");
        getcwd(cwd, 256);
        printf("NEW CWD: %s\n", cwd);
    } else {
        printf("FAILED: chdir\n");
    }

    chdir("/");

    int fd = open("/readme.txt", O_RDONLY);
    if (fd >= 0) {
        struct stat st;
        if (fstat(fd, &st) == 0) {
            printf("FSTAT /readme.txt: Size=%d, Inode=%d\n", (long)st.st_size, (int)st.st_ino);
        } else {
            printf("FAILED: fstat\n");
        }

        char buf[64];
        int bytes = read(fd, buf, 63);
        if (bytes >= 0) {
            buf[bytes] = '\0';
            printf("READ /readme.txt: %s\n", buf);
        } else {
            printf("FAILED: read\n");
        }
        close(fd);
    } else {
        printf("FAILED: open /readme.txt\n");
    }
}

void test_badargs() {
    print_header("TEST: Bad Arguments & Bound Checks");

    int fd = open("/readme.txt", O_RDONLY);
    if (fd >= 0) {
        int ret = read(fd, (void*)0xFFFFFFFFFFFFFFFF, 10);
        printf("READ bad ptr returned: %d\n", ret);

        ret = write(fd, (void*)0xFFFFFFFFFFFFFFFF, 10);
        printf("WRITE bad ptr returned: %d\n", ret);

        struct stat* bad_st = (struct stat*)0x10;
        ret = fstat(fd, bad_st);
        printf("FSTAT bad ptr returned: %d\n", ret);

        close(fd);
    }

    int ret = open((char*)0x0, O_RDONLY);
    printf("OPEN bad ptr returned: %d\n", ret);

    ret = mkdir((char*)0xFFFFFFFFFFFFFFFF, 0755);
    printf("MKDIR bad ptr returned: %d\n", ret);
}

void test_fd_limit() {
    print_header("LIMIT TEST: File Descriptor Exhaustion");

    int fds[128];
    int count = 0;

    for (int i = 0; i < 128; i++) {
        fds[i] = open("/readme.txt", O_RDONLY);
        if (fds[i] < 0) {
            break;
        }
        count++;
    }

    printf("FD LIMIT: Opened %d files before failing.\n", count);

    for (int i = 0; i < count; i++) {
        close(fds[i]);
    }
    printf("CLEANUP: Closed %d files.\n", count);
}

void test_mmap() {
    print_header("TEST: MMAP, MPROTECT, MUNMAP");

    uint64_t length = 4096 * 4;
    void* addr = mmap(0, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if ((int64_t)addr < 0 || addr == 0) {
        printf("FAILED: mmap returned %x\n", (unsigned long)addr);
        return;
    }

    printf("MMAP: Allocated 16KB at 0x%x\n", (unsigned long)addr);

    char* str = (char*)addr;
    const char* msg = "mmap testing string";
    for (int i = 0; msg[i] != '\0'; i++) {
        str[i] = msg[i];
    }
    str[19] = '\0';
    printf("MMAP WRITE/READ: %s\n", str);

    if (mprotect(addr, length, PROT_READ) == 0) {
        printf("MPROTECT: Changed to PROT_READ\n");
    } else {
        printf("FAILED: mprotect\n");
    }

    if (munmap(addr, length) == 0) {
        printf("MUNMAP: Freed 16KB at 0x%x\n", (unsigned long)addr);
    } else {
        printf("FAILED: munmap\n");
    }
}

void test_brk_limit() {
    print_header("LIMIT TEST: BRK OOM & RECOVERY");

    uint64_t initial_brk = (uint64_t)brk(0);
    printf("BRK INITIAL: 0x%x\n", (unsigned long)initial_brk);

    uint64_t current_brk = initial_brk;
    uint64_t increment = 1024 * 1024;
    int allocations = 0;

    printf("ALLOCATING: Increasing heap until OOM...\n");

    while (1) {
        uint64_t new_brk = current_brk + increment;
        uint64_t res = (uint64_t)brk((void*)new_brk);
        if (res != new_brk) {
            break;
        }
        current_brk = new_brk;
        allocations++;

        if (allocations % 10 == 0) {
            printf("PROGRESS: %d MB allocated (Calls: %d)\n", allocations, allocations);
        }
    }

    printf("LIMIT REACHED: %d MB. Successful brk calls: %d\n", allocations, allocations);
    printf("BRK AT LIMIT: 0x%x\n", (unsigned long)brk(0));

    printf("CLEANUP: Resetting brk to initial 0x%x ...\n", (unsigned long)initial_brk);
    uint64_t reset_res = (uint64_t)brk((void*)initial_brk);

    if (reset_res == initial_brk) {
        printf("SUCCESS: Memory released. Current brk: 0x%x\n", (unsigned long)brk(0));
    } else {
        printf("FAILED: Could not reset brk. Returned: 0x%x\n", (unsigned long)reset_res);
    }
}

void test_brk_edge() {
    print_header("TEST: BRK Edge Cases");

    uint64_t initial_brk = (uint64_t)brk(0);
    uint64_t extreme_brk = 0x7FFFFFFFFFFFFFFF;
    uint64_t res = (uint64_t)brk((void*)extreme_brk);
    if (res != extreme_brk) {
        printf("SUCCESS: Kernel rejected extreme brk (Returned 0x%x)\n", (unsigned long)res);
    } else {
        printf("FAILED: Kernel accepted extreme brk!\n");
    }

    uint64_t low_brk = 0x100;
    res = (uint64_t)brk((void*)low_brk);
    if (res != low_brk) {
        printf("SUCCESS: Kernel rejected shrinking below initial brk (Returned 0x%x)\n", (unsigned long)res);
    } else {
        printf("FAILED: Kernel shrunk below heap base!\n");
    }

    brk((void*)initial_brk);
}

void test_fork_limit() {
    print_header("LIMIT TEST: 1000 Process Fork Bomb");

    int pids[1000];
    int spawned = 0;

    printf("SPAWNING: Attempting to fork 1000 children...\n");

    for (int i = 0; i < 1000; i++) {
        pid_t p = fork();

        if (p < 0) {
            printf("FORK LIMIT: Kernel refused fork at child %d\n", spawned);
            break;
        } else if (p == 0) {
            exit(i + 100);
        } else {
            pids[spawned] = p;
            spawned++;
        }
    }

    printf("WAITING: Collecting %d zombies...\n", spawned);

    int reaped = 0;
    int status = 0;
    for (int i = 0; i < spawned; i++) {
        pid_t ret = waitpid(pids[i], &status);
        if (ret == pids[i]) {
            reaped++;
        }
    }

    printf("REAPED: %d children collected. Process limits tested.\n", reaped);
}

void test_fork_tree_recursive(int depth) {
    if (depth == 0) exit(0);

    pid_t p1 = fork();
    if (p1 == 0) {
        test_fork_tree_recursive(depth - 1);
    }

    pid_t p2 = fork();
    if (p2 == 0) {
        test_fork_tree_recursive(depth - 1);
    }

    int st;
    waitpid(p1, &st);
    waitpid(p2, &st);
    exit(0);
}

void test_fork_tree() {
    print_header("TEST: Fork Tree (Scheduler Stress)");

    pid_t p = fork();
    if (p == 0) {
        test_fork_tree_recursive(4);
    } else {
        int st;
        waitpid(p, &st);
        printf("SUCCESS: Fork tree completed. Scheduler and Wait handled nested processes.\n");
    }
}

void test_execve() {
    print_header("TEST: EXECVE (Final Test)");
    printf("EXECVE: Launching /bin/init ...\n");

    const char* argv[] = { "/bin/init", 0 };
    const char* envp[] = { 0 };
    pid_t pid = fork();
    if (pid == 0) {
        int res = execve("/bin/init", (char* const*)argv, (char* const*)envp);
        printf("FAILED: execve returned %d\n", res);
        exit(-1);
    }
    else {
        int status = 0;
        waitpid(pid, &status);
        printf("SUCCESS: execve returned %d\n", status);
    }
}

void _start() {
    printf("\n*** USERSPACE LIMIT & SYSCALL TEST SUITE ***\n");
    printf("PID: %d\n", getpid());
    if (getpid() > 1) {
        printf("Execve worked, exiting...\n");
        exit(0);
    }

    test_basic_fs();
    test_badargs();
    test_fd_limit();
    test_mmap();
    test_brk_limit();
    test_brk_edge();
    test_fork_limit();
    test_fork_tree();
    test_execve();

    exit(0);
}