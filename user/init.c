static long syscall(long num, long a0, long a1, long a2, long a3) {
    register long a7 __asm__("a7") = num;
    register long r0 __asm__("a0") = a0;
    register long r1 __asm__("a1") = a1;
    register long r2 __asm__("a2") = a2;
    register long r3 __asm__("a3") = a3;
    __asm__ volatile("ecall"
        : "+r"(r0)
        : "r"(a7), "r"(r1), "r"(r2), "r"(r3)
        : "memory");
    return r0;
}

#define SYS_OPENAT  56
#define SYS_READ    63
#define SYS_WRITE   64
#define SYS_GETPID  172
#define SYS_FORK    220
#define SYS_EXIT    93

void _start() {
    const char* msg = "Hello from userspace!\n";
    syscall(SYS_WRITE, 1, (long)msg, 22, 0);

    long pid = syscall(SYS_GETPID, 0, 0, 0, 0);

    long child_pid = syscall(SYS_FORK, 0, 0, 0, 0);
    if (child_pid == 0) {
        syscall(SYS_WRITE, 1, (long)"I am child\n", 11, 0);
        char* path = "/readme.txt";
        int fd = syscall(SYS_OPENAT, 0, (long)path, 1, 0);
        char buf[256];
        syscall(SYS_READ, fd, (long)buf, 256, 0);
        syscall(SYS_WRITE, 1, (long)buf, 256, 0);
        syscall(SYS_WRITE, 1, (long)"After write\n", 12, 0);
    } else {
        syscall(SYS_WRITE, 1, (long)"I am parent\n", 12, 0);
    }
    syscall(SYS_EXIT, 0, 0, 0, 0);
}