#ifndef RISC_V_SYSCALL_H
#define RISC_V_SYSCALL_H

#include "../trapframe.h"
#include "../../types.h"

enum Syscall {
    SYS_GETCWD = 17,
    SYS_IOCTL = 29,
    SYS_CHDIR = 49,
    SYS_OPENAT = 56,
    SYS_CLOSE = 57,
    SYS_LSEEK = 62,
    SYS_READ = 63,
    SYS_WRITE = 64,
    SYS_READV = 65,
    SYS_WRITEV = 66,
    SYS_FSTAT = 80,
    SYS_MKDIR = 83,
    SYS_EXIT = 93,
    SYS_EXIT_GROUP = 94,
    SYS_SET_TID_ADDRESS = 96,
    SYS_CLOCK_GETTIME = 113,
    SYS_UNAME = 160,
    SYS_GETPID = 172,
    SYS_GETUID = 174,
    SYS_GETEUID = 175,
    SYS_GETGID = 176,
    SYS_GETEGID = 177,
    SYS_BRK = 214,
    SYS_MUNMAP = 215,
    SYS_FORK = 220,
    SYS_EXECVE = 221,
    SYS_MMAP = 222,
    SYS_MPROTECT = 226,
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
    static uint64_t sys_mmap(TrapFrame* tf);
    static uint64_t sys_munmap(TrapFrame* tf);
    static uint64_t sys_mprotect(TrapFrame* tf);
    static uint64_t sys_getcwd(TrapFrame* tf);
    static uint64_t sys_chdir(TrapFrame* tf);
    static uint64_t sys_mkdir(TrapFrame* tf);
    static uint64_t sys_fstat(TrapFrame* tf);
    static uint64_t sys_exit_group(TrapFrame* tf);
    static uint64_t sys_writev(TrapFrame* tf);
    static uint64_t sys_readv(TrapFrame* tf);
    static uint64_t sys_clock_gettime(TrapFrame* tf);
    static uint64_t sys_set_tid_address(TrapFrame* tf);
    static uint64_t sys_uname(TrapFrame* tf);
    static uint64_t sys_getuid(TrapFrame* tf);
    static uint64_t sys_geteuid(TrapFrame* tf);
    static uint64_t sys_getgid(TrapFrame* tf);
    static uint64_t sys_getegid(TrapFrame* tf);
    static uint64_t sys_lseek(TrapFrame* tf);
};

#endif