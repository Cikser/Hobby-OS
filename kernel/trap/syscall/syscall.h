#ifndef RISC_V_SYSCALL_H
#define RISC_V_SYSCALL_H

#include "../trapframe.h"
#include "../../types.h"

enum Syscall {
    SYS_GETCWD = 17,
    SYS_IOCTL = 29,
    SYS_OPENAT = 56,
    SYS_CLOSE = 57,
    SYS_READ = 63,
    SYS_WRITE = 64,
    SYS_READV = 65,
    SYS_WRITEV = 66,
    SYS_EXIT = 93,
    SYS_EXIT_GROUP = 94,
    SYS_GETPID = 172,
    SYS_BRK = 214,
    SYS_FORK = 220,
    SYS_EXECVE = 221,
    SYS_WAIT4 = 260,
};

class SyscallHandler {
public:
    static void handle(TrapFrame* tf);

private:
    static uint64_t sys_exit(TrapFrame* tf);
    static uint64_t sys_getpid(TrapFrame* tf);
    static uint64_t sys_fork(TrapFrame* tf);
    static uint64_t sys_execve(TrapFrame* tf);
    static uint64_t sys_wait4(TrapFrame* tf);
    static uint64_t sys_read(TrapFrame* tf);
    static uint64_t sys_write(TrapFrame* tf);
    static uint64_t sys_openat(TrapFrame* tf);
    static uint64_t sys_close(TrapFrame* tf);
    static uint64_t sys_brk(TrapFrame* tf);
};

#endif