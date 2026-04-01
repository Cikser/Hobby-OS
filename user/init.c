#include "syscall.h"

static void demo_fs_ops(void) {
    printf("=== FS operations: mkdir, chdir, getcwd ===\n");

    char buf[128];

    if (getcwd(buf, sizeof(buf)) == 0) {
        printf("Current dir: %s\n", buf);
    } else {
        printf("getcwd failed\n");
    }

    if (mkdir("/test_dir", 0) == 0) {
        printf("mkdir /test_dir: OK\n");
    } else {
        printf("mkdir /test_dir: FAIL (možda već postoji?)\n");
    }

    if (chdir("/subdir") == 0) {
        printf("chdir /subdir: OK\n");
        if (getcwd(buf, sizeof(buf)) == 0) {
            printf("Current dir now: %s\n", buf);
        }
    } else {
        printf("chdir /subdir: FAIL\n");
    }

    if (chdir("..") == 0) {
        printf("chdir ..: OK\n");
        getcwd(buf, sizeof(buf));
        printf("Back to: %s\n", buf);
    }

    if (chdir("/test_dir") == 0) {
        printf("chdir /test_dir: OK\n");
        getcwd(buf, sizeof(buf));
        printf("Final path: %s\n", buf);
    } else {
        printf("chdir /test_dir: FAIL\n");
    }
}


void _start(void) {
    printf("Hello from userspace! pid=%ld\n", getpid());

    pid_t child = fork();

    if (child == 0) {
        demo_fs_ops();
        printf("child done, exiting\n");
        exit(0);
    } else {
        int status = 0;
        waitpid(child, &status);
        printf("parent: child exited, status=%d\n", WEXITSTATUS(status));
    }

    exit(0);
}