static long syscall(long num, long a0, long a1, long a2) {
    register long a7 __asm__("a7") = num;
    register long r0 __asm__("a0") = a0;
    register long r1 __asm__("a1") = a1;
    register long r2 __asm__("a2") = a2;
    __asm__ volatile("ecall"
        : "+r"(r0)
        : "r"(a7), "r"(r1), "r"(r2)
        : "memory");
    return r0;
}

#define SYS_WRITE   64
#define SYS_GETPID  172
#define SYS_FORK    220
#define SYS_EXIT    93

void _start() {
    const char* msg = "Hello from userspace!\n";
    syscall(SYS_WRITE, 1, (long)msg, 22);

    long pid = syscall(SYS_GETPID, 0, 0, 0);

    long child_pid = syscall(SYS_FORK, 0, 0, 0);
    if (child_pid == 0) {
        syscall(SYS_WRITE, 1, (long)"I am child\n", 11);
        syscall(SYS_EXIT, 0, 0, 0);
    } else {
        syscall(SYS_WRITE, 1, (long)"I am parent\n", 12);
        syscall(SYS_EXIT, 0, 0, 0);
    }
}