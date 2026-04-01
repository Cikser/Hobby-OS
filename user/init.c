#include "syscall.h"

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
        ssize_t bytes = read(fd, buf, 15);
        if (bytes > 0) {
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

void test_fd_exhaustion() {
    print_header("LIMIT TEST: File Descriptor Exhaustion");

    int fds[128];
    int opened = 0;

    for (int i = 0; i < 128; i++) {
        fds[i] = open("/subdir/nested.txt", O_RDONLY);
        if (fds[i] >= 0) {
            opened++;
        } else {
            break;
        }
    }

    printf("FD LIMIT: Opened %d files before failing.\n", opened);

    for (int i = 0; i < opened; i++) {
        close(fds[i]);
    }
    printf("CLEANUP: Closed %d files.\n", opened);
}

void test_mmap_mprotect() {
    print_header("TEST: MMAP, MPROTECT, MUNMAP");

    size_t size = 4096 * 4;
    void* ptr = mmap((void*)0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr != MAP_FAILED && ptr != (void*)0) {
        printf("MMAP: Allocated 16KB at %p\n", ptr);

        char* str = (char*)ptr;
        strcpy(str, "mmap testing string");
        printf("MMAP WRITE/READ: %s\n", str);

        int prot_res = mprotect(ptr, size, PROT_READ);
        if (prot_res == 0) {
            printf("MPROTECT: Changed to PROT_READ\n");
        } else {
            printf("FAILED: mprotect\n");
        }

        int unmap_res = munmap(ptr, size);
        if (unmap_res == 0) {
            printf("MUNMAP: Freed 16KB at %p\n", ptr);
        } else {
            printf("FAILED: munmap\n");
        }
    } else {
        printf("FAILED: mmap\n");
    }
}

void test_brk_oom() {
    print_header("LIMIT TEST: BRK OOM & RECOVERY");

    void* initial_brk = brk((void*)0);
    printf("BRK INITIAL: %p\n", initial_brk);

    void* current_brk = initial_brk;
    int success_count = 0;
    int mb_allocated = 0;
    size_t chunk_size = 1024 * 1024;

    printf("ALLOCATING: Increasing heap until OOM...\n");

    while (1) {
        void* next_brk = (char*)current_brk + chunk_size;
        void* result = brk(next_brk);

        if (result == current_brk || (long)result == -1 || result == (void*)0) {
            break;
        }

        current_brk = result;
        success_count++;
        mb_allocated++;

        if (mb_allocated % 10 == 0) {
            printf("PROGRESS: %d MB allocated (Calls: %d)\n", mb_allocated, success_count);
        }
    }

    printf("LIMIT REACHED: %d MB. Successful brk calls: %d\n", mb_allocated, success_count);
    printf("BRK AT LIMIT: %p\n", current_brk);

    printf("CLEANUP: Resetting brk to initial %p ...\n", initial_brk);
    void* reset_result = brk(initial_brk);

    if (reset_result == initial_brk) {
        printf("SUCCESS: Memory released. Current brk: %p\n", reset_result);
    } else {
        printf("FAILED: Memory release failed or partially failed. Current brk: %p\n", reset_result);
    }
}

void test_fork_bomb() {
    print_header("LIMIT TEST: 1000 Process Fork Bomb");

    int pids[1000];
    int spawned = 0;
    int target = 1000;

    printf("SPAWNING: Attempting to fork %d children...\n", target);

    for (int i = 0; i < target; i++) {
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

void test_execve() {
    print_header("TEST: EXECVE (Final Test)");
    printf("EXECVE: Launching /bin/init ...\n");

    const char* argv[] = { "/bin/init", 0 };
    const char* envp[] = { 0 };
    pid_t pid = fork();
    if (pid == 0) {
        int res = execve("/bin/init", (char* const*)argv, (char* const*)envp);
        printf("FAILED: execve returned %d\n", res);
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

    if (getpid() > 5) {
        printf("Execve worked, exiting...\n");
        exit(0);
    }

    test_basic_fs();
    test_fd_exhaustion();
    test_mmap_mprotect();
    test_fork_bomb();
    test_brk_oom();

    test_execve();

    exit(0);
}