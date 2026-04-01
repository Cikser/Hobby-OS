#include "syscall.h"

void _start() {
    printf("Hello from userspace!\n");
    pid_t current_pid = getpid();
    printf("Current pid: %ld\n", current_pid);

    pid_t child_pid = fork();
    if (child_pid == 0) {
        for (int i = 0; i < 10; i++) {
            fork();
        }
        int fd = open("/readme.txt", O_RDONLY);
        char buf[256];
        read(fd, buf, 256);
        close(fd);
        printf("readme.txt content: %s\n", buf);
        while (waitpid(-1, 0) > 0);
    }
    else {
        waitpid(-1, 0);
        printf("I am parent\n");
    }
    exit(0);
}