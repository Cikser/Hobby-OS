#include "syscall.h"
#include "../../io/console/console.h"
#include "../../proc/pcb.h"
#include "../../fs/file.h"
#include "../../hw/riscv.h"
#include "../../mm/mem.h"
#include "../../proc/process/process.h"

class Process;

void SyscallHandler::handle(TrapFrame* tf) {
    switch (tf->a7) {
    case SYS_EXIT: tf->a0 = sys_exit(tf); break;
    case SYS_GETPID: tf->a0 = sys_getpid(tf); break;
    case SYS_FORK: tf->a0 = sys_fork(tf); break;
    case SYS_EXECVE:  tf->a0 = sys_execve(tf); break;
    case SYS_WAIT4: tf->a0 = sys_wait4(tf); break;
    case SYS_READ: tf->a0 = sys_read(tf); break;
    case SYS_WRITE: tf->a0 = sys_write(tf); break;
    case SYS_OPENAT: tf->a0 = sys_openat(tf); break;
    case SYS_CLOSE: tf->a0 = sys_close(tf); break;
    case SYS_BRK: tf->a0 = sys_brk(tf); break;
    case SYS_MMAP: tf->a0 = sys_mmap(tf); break;
    case SYS_MUNMAP: tf->a0 = sys_munmap(tf); break;
    case SYS_MPROTECT: tf->a0 = sys_mprotect(tf); break;
    case SYS_GETCWD: tf->a0 = sys_getcwd(tf); break;
    case SYS_CHDIR: tf->a0 = sys_chdir(tf); break;
    case SYS_MKDIR: tf->a0 = sys_mkdir(tf); break;
    case SYS_FSTAT: tf->a0 = sys_fstat(tf); break;
    case SYS_EXIT_GROUP: tf->a0 = sys_exit_group(tf); break;
    default:
        Console::kprintf("unknown syscall: %d\n", tf->a7);
        tf->a0 = -1;
        break;
    }
}

uint64_t SyscallHandler::sys_getpid(TrapFrame* tf) {
    return PCB::running()->pid();
}

uint64_t SyscallHandler::sys_exit(TrapFrame* tf) {
    PCB::runningProcess()->exit((int)(int64_t)tf->a0);
    return 0;
}

uint64_t SyscallHandler::sys_fork(TrapFrame* tf) {
    PCB* child = PCB::runningProcess()->fork();
    return child ? child->pid() : -1;
}

uint64_t SyscallHandler::sys_write(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    uint64_t buf = tf->a1;
    uint64_t len = tf->a2;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return -1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    uint64_t ret = file->write((void*)buf, len);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return ret;
}

uint64_t SyscallHandler::sys_read(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    uint64_t buf = tf->a1;
    uint64_t len = tf->a2;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return -1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    uint64_t ret = file->read((void*)buf, len);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return ret;
}

uint64_t SyscallHandler::sys_brk(TrapFrame* tf) {
    uint64_t newBrk = tf->a0;
    return PCB::runningProcess()->brk(newBrk);
}

uint64_t SyscallHandler::sys_openat(TrapFrame* tf) {
    uint64_t dirfd = tf->a0;
    uint64_t filePath = tf->a1;
    uint64_t flags = tf->a2;
    uint64_t mode = tf->a3;

    char path[256];
    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    auto src = (char*)filePath;
    strcpy(path, src);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return PCB::runningProcess()->openFile(path, flags);
}

uint64_t SyscallHandler::sys_close(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    return PCB::runningProcess()->closeFile(fd);
}

uint64_t SyscallHandler::sys_execve(TrapFrame* tf) {
    uint64_t pathAddr = tf->a0;

    char path[256];
    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    strcpy(path, (char*)pathAddr);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    auto* proc = PCB::runningProcess();

    return proc->exec(path);
}

uint64_t SyscallHandler::sys_wait4(TrapFrame* tf) {
    pid_t pid = tf->a0;
    uint64_t statusAddr = tf->a1;

    auto proc = PCB::runningProcess();

    int status = 0;
    pid_t ret = proc->wait(pid, statusAddr ? &status : nullptr);

    if (statusAddr) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        *(int*)statusAddr = status;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }

    return ret;
}

uint64_t SyscallHandler::sys_mmap(TrapFrame* tf) {
    uint64_t addr = tf->a0;
    uint64_t length = tf->a1;
    auto prot = (uint32_t)tf->a2;
    auto flags = (uint32_t)tf->a3;
    int fd = (int)(int64_t)tf->a4;
    uint64_t offset = tf->a5;

    uint64_t va = PCB::runningProcess()->mmap(addr, length, prot, flags, fd, offset);
    return va;
}

uint64_t SyscallHandler::sys_munmap(TrapFrame* tf) {
    uint64_t addr = tf->a0;
    uint64_t length = tf->a1;
    return (uint64_t)PCB::runningProcess()->munmap(addr, length);
}

uint64_t SyscallHandler::sys_mprotect(TrapFrame* tf) {
    uint64_t addr = tf->a0;
    uint64_t length = tf->a1;
    auto prot = (uint32_t)tf->a2;
    return (uint64_t)PCB::runningProcess()->mprotect(addr, length, prot);
}

uint64_t SyscallHandler::sys_getcwd(TrapFrame* tf) {
    auto buf = (char*)tf->a0;
    uint64_t size = tf->a1;

    if (!buf || size == 0) return (uint64_t)-1;

    char kbuf[256];
    int ret = PCB::runningProcess()->getcwd(kbuf, sizeof(kbuf));
    if (ret < 0) return (uint64_t)-1;

    uint64_t needed = strlen(kbuf) + 1;
    if (needed > size) return (uint64_t)-1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    memcpy(buf, kbuf, needed);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return 0;
}

uint64_t SyscallHandler::sys_chdir(TrapFrame* tf) {
    char path[256];

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    strcpy(path, (char*)tf->a0);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    int ret = PCB::runningProcess()->chdir(path);
    return (ret == 0) ? 0 : (uint64_t)-1;
}

uint64_t SyscallHandler::sys_mkdir(TrapFrame* tf) {
    char path[256];
    auto mode = (uint32_t)tf->a1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    strcpy(path, (char*)tf->a0);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    int ret = PCB::runningProcess()->mkdir(path, mode);
    return (ret == 0) ? 0 : (uint64_t)-1;
}

uint64_t SyscallHandler::sys_fstat(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    auto st = (InodeStat*)tf->a1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    int ret = PCB::runningProcess()->fstat(fd, st);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return (uint64_t)ret;
}

uint64_t SyscallHandler::sys_exit_group(TrapFrame* tf) {
    PCB::runningProcess()->exitGroup((int)(int64_t)tf->a0);
    return 0;
}

struct iovec {
    void* iov_base;
    uint64_t iov_len;
};

uint64_t SyscallHandler::sys_writev(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    auto* iov = (iovec*)tf->a1;
    int iovcnt = (int)(int64_t)tf->a2;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return -1;

    uint64_t total = 0;
    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    for (int i = 0; i < iovcnt; i++) {
        if (!iov[i].iov_base || iov[i].iov_len == 0) continue;
        uint64_t written = file->write(iov[i].iov_base, iov[i].iov_len);
        if ((int64_t)written < 0) {
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM); return -1;
        }
        total += written;
    }
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    return total;
}

uint64_t SyscallHandler::sys_readv(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    auto* iov = (iovec*)tf->a1;
    int iovcnt = (int)(int64_t)tf->a2;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return -1;

    uint64_t total = 0;
    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    for (int i = 0; i < iovcnt; i++) {
        if (!iov[i].iov_base || iov[i].iov_len == 0) continue;
        uint64_t read = file->read(iov[i].iov_base, iov[i].iov_len);
        if ((int64_t)read < 0) {
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM); return -1;
        }
        total += read;
        if (read < iov[i].iov_len) break;
    }
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    return total;
}