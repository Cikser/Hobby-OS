#ifndef RISC_V_SYSCALL_H
#define RISC_V_SYSCALL_H

#include "../trapframe.h"
#include "../../types.h"

enum Syscall {
    SYS_GETCWD = 17,
    SYS_DUP = 23,
    SYS_DUP3 = 24,
    SYS_FCNTL = 25,
    SYS_IOCTL = 29,
    SYS_UNLINKAT = 35,
    SYS_FTRUNCATE = 46,
    SYS_CHDIR = 49,
    SYS_OPENAT = 56,
    SYS_CLOSE = 57,
    SYS_GETDENTS64 = 61,
    SYS_LSEEK = 62,
    SYS_READ = 63,
    SYS_WRITE = 64,
    SYS_READV = 65,
    SYS_WRITEV = 66,
    SYS_PREAD64 = 67,
    SYS_PWRITE64 = 68,
    SYS_READLINKAT = 78,
    SYS_NEWFSTATAT = 79,
    SYS_FSTAT = 80,
    SYS_MKDIR = 83,
    SYS_EXIT = 93,
    SYS_EXIT_GROUP = 94,
    SYS_SET_TID_ADDRESS = 96,
    SYS_FUTEX = 98,
    SYS_NANOSLEEP = 101,
    SYS_CLOCK_GETTIME = 113,
    SYS_CLOCK_GETRES = 114,
    SYS_SCHED_YIELD = 124,
    SYS_UNAME = 160,
    SYS_GETRUSAGE = 165,
    SYS_UMASK = 166,
    SYS_GETTIMEOFDAY = 169,
    SYS_GETPID = 172,
    SYS_GETPPID = 173,
    SYS_GETUID = 174,
    SYS_GETEUID = 175,
    SYS_GETGID = 176,
    SYS_GETEGID = 177,
    SYS_GETTID = 178,
    SYS_BRK = 214,
    SYS_MUNMAP = 215,
    SYS_CLONE = 220,
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
    static uint64_t sys_clone(TrapFrame* tf);
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
    static uint64_t sys_unlinkat(TrapFrame* tf);
    static uint64_t sys_futex(TrapFrame* tf);
    static uint64_t sys_sched_yield(TrapFrame* tf);
    static uint64_t sys_gettid(TrapFrame* tf);
    static uint64_t sys_nanosleep(TrapFrame* tf);
    static uint64_t sys_clock_getres(TrapFrame* tf);
    static uint64_t sys_getppid(TrapFrame* tf);
    static uint64_t sys_gettimeofday(TrapFrame* tf);
    static uint64_t sys_getrusage(TrapFrame* tf);
    static uint64_t sys_umask(TrapFrame* tf);
    static uint64_t sys_fcntl(TrapFrame* tf);
    static uint64_t sys_getdents64(TrapFrame* tf);
    static uint64_t sys_readlinkat(TrapFrame* tf);
    static uint64_t sys_newfstatat(TrapFrame* tf);
    static uint64_t sys_pread64(TrapFrame* tf);
    static uint64_t sys_pwrite64(TrapFrame* tf);
    static uint64_t sys_dup(TrapFrame* tf);
    static uint64_t sys_dup3(TrapFrame* tf);
    static uint64_t sys_ftruncate(TrapFrame* tf);
};

#endif