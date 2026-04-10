#include "syscall.h"
#include "../trap.h"
#include "../../io/console/console.h"
#include "../../proc/pcb.h"
#include "../../fs/file.h"
#include "../../hw/riscv.h"
#include "../../mm/mem.h"
#include "../../proc/process/process.h"
#include "../../fs/path_utils.h"
#include "../../fs/vfs.h"
#include "../../proc/sync/futex.h"
#include "../../proc/thread/thread.h"

void SyscallHandler::handle(TrapFrame* tf) {
    switch (tf->a7) {
    case SYS_EXIT: tf->a0 = sys_exit(tf); break;
    case SYS_GETPID: tf->a0 = sys_getpid(tf); break;
    case SYS_CLONE: tf->a0 = sys_clone(tf); break;
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
    case SYS_CLOCK_GETTIME: tf->a0 = sys_clock_gettime(tf); break;
    case SYS_WRITEV: tf->a0 = sys_writev(tf); break;
    case SYS_READV: tf->a0 = sys_readv(tf); break;
    case SYS_SET_TID_ADDRESS: tf->a0 = sys_set_tid_address(tf); break;
    case SYS_UNAME: tf->a0 = sys_uname(tf); break;
    case SYS_GETUID: tf->a0 = sys_getuid(tf); break;
    case SYS_GETEUID: tf->a0 = sys_geteuid(tf); break;
    case SYS_GETGID: tf->a0 = sys_getgid(tf); break;
    case SYS_GETEGID: tf->a0 = sys_getegid(tf); break;
    case SYS_LSEEK: tf->a0 = sys_lseek(tf); break;
    case SYS_UNLINKAT: tf->a0 = sys_unlinkat(tf); break;
    case SYS_FUTEX: tf->a0 = sys_futex(tf); break;
    case SYS_SCHED_YIELD: tf->a0 = sys_sched_yield(tf); break;
    case SYS_GETTID: tf->a0 = sys_gettid(tf); break;
    default:
        Console::kprintf("unknown syscall: %d\n", tf->a7);
        tf->a0 = -1;
        break;
    }
}

static char* copyPathFromUser(uint64_t userAddr) {
    if (!userAddr) return nullptr;

    if (!PCB::runningProcess()->checkOperation(userAddr, 1, SegmentDesc::SEG_R))
        return nullptr;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    char* result = kstrdup_user((const char*)userAddr, PATH_MAX);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    if (!PCB::runningProcess()->checkOperation(userAddr, strlen(result), SegmentDesc::SEG_R))
        return nullptr;

    return result;
}

uint64_t SyscallHandler::sys_getpid(TrapFrame* tf) {
    return PCB::runningProcess()->pid();
}

uint64_t SyscallHandler::sys_exit(TrapFrame* tf) {
    PCB::running()->exit((int)(int64_t)tf->a0);
    return 0;
}

uint64_t SyscallHandler::sys_clone(TrapFrame* tf) {
    uint64_t flags = tf->a0;
    uint64_t childStack = tf->a1;
    auto* parentTid = (int*)tf->a2;
    uint64_t tls = tf->a3;
    auto* childTid = (int*)tf->a4;

    Process* proc = PCB::runningProcess();

    if (flags & CLONE_THREAD) {
        if (!(flags & CLONE_VM))    return (uint64_t)-1;
        if (childStack == 0)        return (uint64_t)-1;

        if (parentTid &&
            !proc->checkOperation((uint64_t)parentTid, sizeof(int), SegmentDesc::SEG_W))
            return (uint64_t)-1;

        if (childTid &&
            !proc->checkOperation((uint64_t)childTid, sizeof(int), SegmentDesc::SEG_W))
            return (uint64_t)-1;

        int* clearTid = (flags & CLONE_CHILD_CLEARTID) ? childTid : nullptr;

        uint64_t entry = tf->sepc;

        Thread* t = proc->cloneThread(entry, childStack, tls,
                                       parentTid, childTid, clearTid);
        if (!t) return (uint64_t)-1;

        return t->pid();
    }

    Process* child = proc->fork();
    if (!child) return (uint64_t)-1;

    return child->pid();
}

uint64_t SyscallHandler::sys_write(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    uint64_t buf = tf->a1;
    uint64_t len = tf->a2;

    if (!PCB::runningProcess()->checkOperation(buf, len, SegmentDesc::SEG_R))
        return -1;

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

    if (!PCB::runningProcess()->checkOperation(buf, len, SegmentDesc::SEG_W))
        return -1;

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

    AutoPath path(copyPathFromUser(filePath));
    if (!path.valid()) return -1;

    return PCB::runningProcess()->openFile(path, flags);
}

uint64_t SyscallHandler::sys_close(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    return PCB::runningProcess()->closeFile(fd);
}

uint64_t SyscallHandler::sys_execve(TrapFrame* tf) {
    uint64_t pathAddr = tf->a0;

    AutoPath path(copyPathFromUser(pathAddr));
    if (!path.valid()) return -1;

    return PCB::runningProcess()->exec(path);
}

uint64_t SyscallHandler::sys_wait4(TrapFrame* tf) {
    pid_t pid = tf->a0;
    uint64_t statusAddr = tf->a1;

    if (statusAddr && !PCB::runningProcess()->checkOperation(statusAddr, sizeof(int), SegmentDesc::SEG_W))
        return -1;

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

    if (!PCB::runningProcess()->checkOperation((uint64_t)buf, size, SegmentDesc::SEG_W))
        return -1;

    char* kbuf = PCB::runningProcess()->cwd();
    if (!kbuf) return (uint64_t)-1;

    uint64_t needed = strlen(kbuf) + 1;
    if (needed > size) {
        MemoryAllocator::kfree(kbuf);
        return (uint64_t)-1;
    }

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    memcpy(buf, kbuf, needed);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    MemoryAllocator::kfree(kbuf);
    return 0;
}

uint64_t SyscallHandler::sys_chdir(TrapFrame* tf) {
    AutoPath path(copyPathFromUser(tf->a0));
    if (!path.valid()) return -1;

    int ret = PCB::runningProcess()->chdir(path);
    return (ret == 0) ? 0 : (uint64_t)-1;
}

uint64_t SyscallHandler::sys_mkdir(TrapFrame* tf) {
    auto mode = (uint32_t)tf->a1;

    AutoPath path(copyPathFromUser(tf->a0));
    if (!path.valid()) return -1;

    int ret = PCB::runningProcess()->mkdir(path, mode);
    return (ret == 0) ? 0 : (uint64_t)-1;
}

uint64_t SyscallHandler::sys_fstat(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    auto st = (InodeStat*)tf->a1;

    if (!PCB::runningProcess()->checkOperation((uint64_t)st, sizeof(InodeStat), SegmentDesc::SEG_W))
        return -1;

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

    if (!PCB::runningProcess()->checkOperation((uint64_t)iov, (uint64_t)iovcnt * sizeof(iovec), SegmentDesc::SEG_R))
        return -1;

    uint64_t total = 0;
    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    for (int i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len == 0) continue;
        if (!iov[i].iov_base) {
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return -1;
        }
        if (!PCB::runningProcess()->checkOperation((uint64_t)iov[i].iov_base, iov[i].iov_len, SegmentDesc::SEG_R)) {
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return total ? total : -1;
        }
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

    if (!PCB::runningProcess()->checkOperation((uint64_t)iov, (uint64_t)iovcnt * sizeof(iovec), SegmentDesc::SEG_R))
        return -1;

    uint64_t total = 0;
    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    for (int i = 0; i < iovcnt; i++) {
        if (iov[i].iov_len == 0) continue;
        if (!iov[i].iov_base) {
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return -1;
        }
        if (!PCB::runningProcess()->checkOperation((uint64_t)iov[i].iov_base, iov[i].iov_len, SegmentDesc::SEG_R)) {
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return total ? total : -1;
        }
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

struct timespec {
    time_t tv_sec;
    time_t tv_nsec;
};

uint64_t SyscallHandler::sys_clock_gettime(TrapFrame* tf) {
    auto clockid = (int32_t)tf->a0;
    auto* ts = (timespec*)tf->a1;
    if (!ts) return -1;

    if (!PCB::runningProcess()->checkOperation((uint64_t)ts, sizeof(timespec), SegmentDesc::SEG_W))
        return -1;

    uint64_t ms = TrapHandler::getTicks();

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    ts->tv_sec = ms / 1000;
    ts->tv_nsec = (ms % 1000) * 1000000LL;
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return 0;
}

uint64_t SyscallHandler::sys_set_tid_address(TrapFrame* tf) {
    uint64_t tidptr = tf->a0;
    PCB::running()->setTidAddress(tidptr);
    return PCB::running()->pid();
}

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

uint64_t SyscallHandler::sys_uname(TrapFrame* tf) {
    auto* buf = (utsname*)tf->a0;
    if (!buf) return -1;

    if (!PCB::runningProcess()->checkOperation((uint64_t)buf, sizeof(utsname), SegmentDesc::SEG_W))
        return -1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    strcpy(buf->sysname,"Kohor");
    strcpy(buf->nodename,"kernel");
    strcpy(buf->release,"0.0.1");
    strcpy(buf->version,"#1");
    strcpy(buf->machine,"riscv64");
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return 0;
}

uint64_t SyscallHandler::sys_getuid(TrapFrame* tf) {
    return 0;
}

uint64_t SyscallHandler::sys_geteuid(TrapFrame* tf) {
    return 0;
}

uint64_t SyscallHandler::sys_getgid(TrapFrame* tf) {
    return 0;
}

uint64_t SyscallHandler::sys_getegid(TrapFrame* tf) {
    return 0;
}

uint64_t SyscallHandler::sys_lseek(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    auto off = (int64_t)tf->a1;
    int whence  = (int)(int64_t)tf->a2;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return -1;

    int64_t ret = file->seek(off, whence);
    return (uint64_t)ret;
}

uint64_t SyscallHandler::sys_unlinkat(TrapFrame* tf) {
    int dirfd = (int)(int64_t)tf->a0;
    uint64_t buf = tf->a1;
    uint32_t flags = tf->a2;

    AutoPath path(copyPathFromUser(buf));
    if (!path.valid()) return -1;

    int ret = VFS::unlink(path, flags);
    return ret;
}

uint64_t SyscallHandler::sys_futex(TrapFrame* tf) {
    auto* uaddr = (uint32_t*)tf->a0;
    auto op = (int)(int64_t)tf->a1;
    auto val = (uint32_t)tf->a2;
    auto timeout = (time_t)tf->a3;

    if (!PCB::runningProcess()->checkOperation((uint64_t)uaddr, sizeof(uint32_t), SegmentDesc::SEG_R))
        return (uint64_t)FUTEX_EINVAL;

    return (uint64_t)Futex::syscall(uaddr, op, val, timeout);
}

uint64_t SyscallHandler::sys_sched_yield(TrapFrame* tf) {
    PCB::yield();
    return 0;
}

uint64_t SyscallHandler::sys_gettid(TrapFrame* tf) {
    return PCB::running()->pid();
}
