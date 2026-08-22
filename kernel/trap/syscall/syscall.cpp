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
#include "../../fs/pipe/pipe.h"
#include "../../io/terminal/termios.h"

static constexpr int LINUX_EFAULT = 14;
static constexpr int LINUX_ENOENT = 2;

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
    case SYS_NANOSLEEP: tf->a0 = sys_nanosleep(tf); break;
    case SYS_CLOCK_GETRES: tf->a0 = sys_clock_getres(tf); break;
    case SYS_GETPPID: tf->a0 = sys_getppid(tf); break;
    case SYS_GETTIMEOFDAY: tf->a0 = sys_gettimeofday(tf); break;
    case SYS_GETRUSAGE: tf->a0 = sys_getrusage(tf); break;
    case SYS_UMASK: tf->a0 = sys_umask(tf); break;
    case SYS_FCNTL: tf->a0 = sys_fcntl(tf); break;
    case SYS_GETDENTS64: tf->a0 = sys_getdents64(tf); break;
    case SYS_READLINKAT: tf->a0 = sys_readlinkat(tf); break;
    case SYS_NEWFSTATAT: tf->a0 = sys_newfstatat(tf); break;
    case SYS_PREAD64: tf->a0 = sys_pread64(tf); break;
    case SYS_PWRITE64: tf->a0 = sys_pwrite64(tf); break;
    case SYS_DUP: tf->a0 = sys_dup(tf); break;
    case SYS_DUP3: tf->a0 = sys_dup3(tf); break;
    case SYS_FTRUNCATE: tf->a0 = sys_ftruncate(tf); break;
    case SYS_KILL: tf->a0 = sys_kill(tf); break;
    case SYS_TKILL: tf->a0 = sys_tkill(tf); break;
    case SYS_TGKILL: tf->a0 = sys_tgkill(tf); break;
    case SYS_SIGALTSTACK: tf->a0 = sys_sigaltstack(tf); break;
    case SYS_RT_SIGACTION: tf->a0 = sys_rt_sigaction(tf); break;
    case SYS_RT_SIGPROCMASK: tf->a0 = sys_rt_sigprocmask(tf); break;
    case SYS_RT_SIGPENDING: tf->a0 = sys_rt_sigpending(tf); break;
    case SYS_RT_SIGRETURN: tf->a0 = sys_rt_sigreturn(tf); break;
    case SYS_MADVISE: tf->a0 = sys_madvise(tf); break;
    case SYS_PRLIMIT64: tf->a0 = sys_prlimit64(tf); break;
    case SYS_GETRANDOM: tf->a0 = sys_getrandom(tf); break;
    case SYS_RSEQ: tf->a0 = (uint64_t)-38; break;
    case SYS_MEMBARRIER: tf->a0 = 0; break;
    case SYS_STATX: tf->a0 = sys_statx(tf); break;
    case SYS_IOCTL: tf->a0 = sys_ioctl(tf); break;
    case SYS_PIPE2: tf->a0 = sys_pipe2(tf); break;
    case SYS_SETPGID: tf->a0 = sys_setpgid(tf); break;
    case SYS_GETPGID: tf->a0 = sys_getpgid(tf); break;
    case SYS_SETSID: tf->a0 = sys_setsid(tf); break;
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

struct AutoArgv {
    char** args{nullptr};

    explicit AutoArgv(char** a) : args(a) {}
    ~AutoArgv() {
        if (args) {
            for (int i = 0; args[i] != nullptr; i++) {
                MemoryAllocator::kfree(args[i]);
            }
            MemoryAllocator::kfree(args);
        }
    }

    AutoArgv(const AutoArgv&) = delete;
    AutoArgv& operator=(const AutoArgv&) = delete;

    operator char**() const { return args; }
    bool valid() const { return args != nullptr; }
};

static char** copyArgvFromUser(uint64_t userAddr) {
    if (!userAddr) return nullptr;

    auto process = PCB::runningProcess();

    if (!process->checkOperation(userAddr, sizeof(uint64_t), SegmentDesc::SEG_R))
        return nullptr;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);

    uint64_t* userPtrArray = (uint64_t*)userAddr;
    int count = 0;
    
    const int MAX_ARGS = 256; 
    while (userPtrArray[count] != 0) {
        count++;
        if (count > MAX_ARGS) {
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return nullptr;
        }
    }

    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    if (!process->checkOperation(userAddr, (count + 1) * sizeof(uint64_t), SegmentDesc::SEG_R))
        return nullptr;

    char** kargs = (char**)MemoryAllocator::kmalloc((count + 1) * sizeof(char*));
    if (!kargs) return nullptr;

    for (int i = 0; i < count; i++) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        uint64_t strAddr = userPtrArray[i];
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

        if (!strAddr) {
            kargs[i] = nullptr;
            break;
        }

        if (!process->checkOperation(strAddr, 1, SegmentDesc::SEG_R)) {
            for (int j = 0; j < i; j++) MemoryAllocator::kfree(kargs[j]);
            MemoryAllocator::kfree(kargs);
            return nullptr;
        }

        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        kargs[i] = kstrdup_user((const char*)strAddr, PATH_MAX);
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

        if (!kargs[i]) {
            for (int j = 0; j < i; j++) MemoryAllocator::kfree(kargs[j]);
            MemoryAllocator::kfree(kargs);
            return nullptr;
        }

        if (!process->checkOperation(strAddr, strlen(kargs[i]), SegmentDesc::SEG_R)) {
            for (int j = 0; j <= i; j++) MemoryAllocator::kfree(kargs[j]);
            MemoryAllocator::kfree(kargs);
            return nullptr;
        }
    }

    kargs[count] = nullptr;
    return kargs;
}

static bool validateUserBufferByPte(Process* proc, uint64_t addr, uint64_t len, uint32_t op) {
    if (!proc) return false;
    if (len == 0) return true;
    if (addr > addr + len - 1) return false;

    const uint64_t pageMask = MemoryLayout::PAGE_SIZE - 1;
    uint64_t page = addr & ~pageMask;
    uint64_t last = (addr + len - 1) & ~pageMask;

    while (true) {
        uint64_t flags = proc->pmt()->getFlags(page);
        if (!(flags & PMT::PAGE_V) || !(flags & PMT::PAGE_U) || (flags & op) != op)
            return false;
        if (page == last) break;
        page += MemoryLayout::PAGE_SIZE;
    }
    return true;
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
        if (!(flags & CLONE_VM)) return (uint64_t)-1;
        if (childStack == 0) return (uint64_t)-1;

        if (parentTid &&
            !proc->checkOperation((uint64_t)parentTid, sizeof(int), SegmentDesc::SEG_W) &&
            !validateUserBufferByPte(proc, (uint64_t)parentTid, sizeof(int), SegmentDesc::SEG_W))
            return (uint64_t)-1;

        if (childTid &&
            !proc->checkOperation((uint64_t)childTid, sizeof(int), SegmentDesc::SEG_W) &&
            !validateUserBufferByPte(proc, (uint64_t)childTid, sizeof(int), SegmentDesc::SEG_W))
            return (uint64_t)-1;

        int* clearTid = (flags & CLONE_CHILD_CLEARTID) ? childTid : nullptr;

        uint64_t entry = tf->sepc;

        Thread* t = proc->cloneThread(entry, childStack, tls,
                                       parentTid, childTid, clearTid);
        if (!t) return (uint64_t)-1;
        memcpy(t->m_trapFrame, tf, sizeof(TrapFrame));
        t->m_trapFrame->sepc = entry;
        t->m_trapFrame->sp = childStack;
        if (flags & CLONE_SETTLS)
            t->m_trapFrame->tp = tls;
        t->m_trapFrame->a0 = 0;
        t->m_trapFrame->kstack = (uint64_t)t->m_kstack + PCB::KERNEL_STACK_SIZE;
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

    Process* proc = PCB::runningProcess();
    if (!proc->checkOperation(buf, len, SegmentDesc::SEG_W) &&
        !validateUserBufferByPte(proc, buf, len, SegmentDesc::SEG_W))
        return -1;

    File* file = proc->getFile(fd);
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
    if (!path.valid()) return (uint64_t)-LINUX_EFAULT;

    uint64_t fd = PCB::runningProcess()->openFile(path, flags);
    if ((int64_t)fd < 0) return (uint64_t)-LINUX_ENOENT;
    return fd;
}

uint64_t SyscallHandler::sys_close(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    return PCB::runningProcess()->closeFile(fd);
}

uint64_t SyscallHandler::sys_execve(TrapFrame* tf) {
    uint64_t pathAddr = tf->a0;
    uint64_t argvAddr = tf->a1;
    uint64_t envpAddr = tf->a2;

    AutoPath path(copyPathFromUser(pathAddr));
    if (!path.valid()) return -1;

    AutoArgv argv(copyArgvFromUser(argvAddr));
    AutoArgv envp(copyArgvFromUser(envpAddr));
    
    return PCB::runningProcess()->exec(path, argv, envp);
}

uint64_t SyscallHandler::sys_wait4(TrapFrame* tf) {
    pid_t pid = tf->a0;
    uint64_t statusAddr = tf->a1;
    int options = (int)(int64_t)tf->a2;

    if (statusAddr && !PCB::runningProcess()->checkOperation(statusAddr, sizeof(int), SegmentDesc::SEG_W))
        return -1;

    auto proc = PCB::runningProcess();

    int status = 0;
    pid_t ret = proc->wait(pid, statusAddr ? &status : nullptr, options);

    if (ret > 0 && statusAddr) {
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

uint64_t SyscallHandler::sys_nanosleep(TrapFrame* tf) {
    auto* req = (timespec*)tf->a0;
    auto* rem = (timespec*)tf->a1;

    if (!req) return (uint64_t)-1;

    if (!PCB::runningProcess()->checkOperation((uint64_t)req, sizeof(timespec), SegmentDesc::SEG_R))
        return (uint64_t)-1;

    if (rem && !PCB::runningProcess()->checkOperation((uint64_t)rem, sizeof(timespec), SegmentDesc::SEG_W))
        return (uint64_t)-1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    time_t sec = req->tv_sec;
    time_t nsec = req->tv_nsec;
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    if (sec < 0 || nsec < 0 || nsec >= 1000000000LL)
        return (uint64_t)-22;

    time_t ticks = (sec * 1000 + (nsec + 999999) / 1000000);

    if (ticks > 0)
        PCB::sleep(ticks);

    if (rem) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }

    return 0;
}

uint64_t SyscallHandler::sys_clock_getres(TrapFrame* tf) {
    auto* res = (timespec*)tf->a1;

    if (!res) return 0;

    if (!PCB::runningProcess()->checkOperation((uint64_t)res, sizeof(timespec), SegmentDesc::SEG_W))
        return (uint64_t)-1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    res->tv_sec = 0;
    res->tv_nsec = 1000000;
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return 0;
}

static constexpr int AT_EMPTY_PATH  = 0x1000;
static constexpr int AT_SYMLINK_NOFOLLOW = 0x100;

uint64_t SyscallHandler::sys_newfstatat(TrapFrame* tf) {
    int dirfd = (int)(int64_t)tf->a0;
    uint64_t pathAddr = tf->a1;
    auto* statbuf = (InodeStat*)tf->a2;
    int flags = (int)(int64_t)tf->a3;

    if (!statbuf) return (uint64_t)-1;
    if (!PCB::runningProcess()->checkOperation((uint64_t)statbuf, sizeof(InodeStat), SegmentDesc::SEG_W))
        return (uint64_t)-1;

    VfsInode* inode = nullptr;

    if (flags & AT_EMPTY_PATH) {
        if (dirfd == -100) {
            inode = VFS::getInode(2);
        }
        else {
            File* f = PCB::runningProcess()->getFile(dirfd);
            if (!f) return (uint64_t)-1;
            RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
            int ret = f->fstat(statbuf);
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return ret == 0 ? 0 : (uint64_t)-1;
        }
    }
    else {
        AutoPath path(copyPathFromUser(pathAddr));
        if (!path.valid()) return (uint64_t)-1;

        inode = VFS::resolvePath(path);
    }

    if (!inode) return (uint64_t)-1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    int ret = inode->stat(statbuf);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    uint32_t inum = inode->inodeNum();
    VFS::putInode(inode, inum);

    return ret == 0 ? 0 : (uint64_t)-1;
}

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[1];
};

static constexpr uint8_t DT_UNKNOWN = 0;
static constexpr uint8_t DT_REG = 8;
static constexpr uint8_t DT_DIR = 4;

uint64_t SyscallHandler::sys_getdents64(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    uint64_t buf = tf->a1;
    uint64_t count = tf->a2;

    if (count == 0) return 0;

    if (!PCB::runningProcess()->checkOperation(buf, count, SegmentDesc::SEG_W))
        return (uint64_t)-1;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return (uint64_t)-1;

    uint64_t startIdx = file->tell();

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);

    uint64_t written = 0;
    uint64_t idx = startIdx;

    while (written < count) {
        DirEntry entry;
        InodeStat st;
        int sret = file->fstat(&st);
        if (sret != 0) break;

        VfsInode* inode = VFS::getInode((uint32_t)st.st_ino);
        if (!inode) break;

        int ret = inode->readdir((uint32_t)idx, &entry);
        VFS::putInode(inode, (uint32_t)st.st_ino);

        if (ret != 0) break;

        uint64_t namelen = 0;
        while (entry.name[namelen]) namelen++;

        uint64_t reclen = sizeof(linux_dirent64) - 1 + namelen + 1;
        reclen = (reclen + 7) & ~7ULL;

        if (written + reclen > count) {
            if (written == 0) {
                RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
                return (uint64_t)-1;
            }
            break;
        }

        auto* d = (linux_dirent64*)(buf + written);
        d->d_ino = entry.inodeNum;
        d->d_off = (int64_t)(idx + 1);
        d->d_reclen = (uint16_t)reclen;
        d->d_type = DT_UNKNOWN;
        memcpy(d->d_name, entry.name, namelen + 1);

        written += reclen;
        idx++;
    }

    file->seek((int64_t)idx, File::SEEK_SET);

    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    return written;
}

uint64_t SyscallHandler::sys_readlinkat(TrapFrame* tf) {
    int dirfd = (int)(int64_t)tf->a0;
    uint64_t pathAddr = tf->a1;
    uint64_t buf = tf->a2;
    uint64_t bufsize = tf->a3;

    if (!buf || bufsize == 0) return (uint64_t)-1;

    AutoPath path(copyPathFromUser(pathAddr));
    if (!path.valid()) return (uint64_t)-1;

    if (!PCB::runningProcess()->checkOperation(buf, bufsize, SegmentDesc::SEG_W))
        return (uint64_t)-1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    if (strcmp((const char*)path, "/proc/self/exe") == 0 ||
        strcmp((const char*)path, "/proc/self/fd/0") == 0) {
        const char* fake = "/bin/init";
        uint64_t len = 0;
        while (fake[len]) len++;
        if (len > bufsize) len = bufsize;
        memcpy((void*)buf, fake, len);
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
        return (int64_t)len;
    }
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return (uint64_t)-1;
}

static constexpr int F_DUPFD = 0;
static constexpr int F_GETFD = 1;
static constexpr int F_SETFD = 2;
static constexpr int F_GETFL = 3;
static constexpr int F_SETFL = 4;
static constexpr int FD_CLOEXEC = 1;
static constexpr int O_NONBLOCK = 0x800;

uint64_t SyscallHandler::sys_fcntl(TrapFrame* tf) {
    int fd  = (int)(int64_t)tf->a0;
    int cmd = (int)(int64_t)tf->a1;
    int arg = (int)(int64_t)tf->a2;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return (uint64_t)-1;

    switch (cmd) {
        case F_GETFD:
            return 0;

        case F_SETFD:
            return 0;

        case F_GETFL:
            return (uint64_t)file->flags();

        case F_SETFL:
            return 0;

        case F_DUPFD: {
            File* copy = new File(*file, true);
            if (!copy) return (uint64_t)-1;
            int newfd = PCB::runningProcess()->m_fdTable->alloc(copy);
            if (newfd < 0) {
                copy->close();
                delete copy;
                return (uint64_t)-1;
            }
            return (uint64_t)newfd;
        }

        default:
            return 0;
    }
}

uint64_t SyscallHandler::sys_pread64(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    uint64_t buf = tf->a1;
    uint64_t count = tf->a2;
    int64_t offset = (int64_t)tf->a3;

    if (offset < 0) return (uint64_t)-1;
    if (count == 0) return 0;

    if (!PCB::runningProcess()->checkOperation(buf, count, SegmentDesc::SEG_W))
        return (uint64_t)-1;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return (uint64_t)-1;

    int64_t saved = (int64_t)file->tell();

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    int64_t seekret = file->seek(offset, File::SEEK_SET);
    uint64_t ret = (uint64_t)-1;
    if (seekret >= 0) {
        int n = file->read((void*)buf, count);
        ret = (n >= 0) ? (uint64_t)n : (uint64_t)-1;
    }
    file->seek(saved, File::SEEK_SET);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return ret;
}

uint64_t SyscallHandler::sys_pwrite64(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    uint64_t buf = tf->a1;
    uint64_t count = tf->a2;
    int64_t offset = (int64_t)tf->a3;

    if (offset < 0) return (uint64_t)-1;
    if (count == 0) return 0;

    if (!PCB::runningProcess()->checkOperation(buf, count, SegmentDesc::SEG_R))
        return (uint64_t)-1;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return (uint64_t)-1;

    int64_t saved = (int64_t)file->tell();

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    int64_t seekret = file->seek(offset, File::SEEK_SET);
    uint64_t ret = (uint64_t)-1;
    if (seekret >= 0) {
        int n = file->write((void*)buf, count);
        ret = (n >= 0) ? (uint64_t)n : (uint64_t)-1;
    }
    file->seek(saved, File::SEEK_SET);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return ret;
}

uint64_t SyscallHandler::sys_dup(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return (uint64_t)-1;

    File* copy = new File(*file, true);
    if (!copy) return (uint64_t)-1;

    int newfd = PCB::runningProcess()->m_fdTable->alloc(copy);
    if (newfd < 0) {
        copy->close();
        delete copy;
        return (uint64_t)-1;
    }
    return (uint64_t)newfd;
}

uint64_t SyscallHandler::sys_dup3(TrapFrame* tf) {
    int oldfd = (int)(int64_t)tf->a0;
    int newfd = (int)(int64_t)tf->a1;

    if (oldfd == newfd) return (uint64_t)-1;
    if (newfd < 0 || newfd >= FdTable::MAX_FDS) return (uint64_t)-1;

    File* file = PCB::runningProcess()->getFile(oldfd);
    if (!file) return (uint64_t)-1;

    File* copy = new File(*file, true);
    if (!copy) return (uint64_t)-1;

    int got = PCB::runningProcess()->m_fdTable->allocAt(newfd, copy);
    if (got < 0) {
        copy->close();
        delete copy;
        return (uint64_t)-1;
    }
    return (uint64_t)newfd;
}

uint64_t SyscallHandler::sys_ftruncate(TrapFrame* tf) {
    int fd = (int)(int64_t)tf->a0;
    int64_t len = (int64_t)tf->a1;

    if (len < 0) return (uint64_t)-1;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return (uint64_t)-1;

    InodeStat st;
    if (file->fstat(&st) != 0) return (uint64_t)-1;

    VfsInode* inode = VFS::getInode((uint32_t)st.st_ino);
    if (!inode) return (uint64_t)-1;

    int ret = inode->truncate((uint64_t)len);
    VFS::putInode(inode, (uint32_t)st.st_ino);

    return ret == 0 ? 0 : (uint64_t)-1;
}

uint64_t SyscallHandler::sys_getppid(TrapFrame* tf) {
    return PCB::runningProcess()->ppid();
}

struct timeval_t {
    int64_t tv_sec;
    int64_t tv_usec;
};

uint64_t SyscallHandler::sys_gettimeofday(TrapFrame* tf) {
    auto* tv = (timeval_t*)tf->a0;

    if (!tv) return 0;

    if (!PCB::runningProcess()->checkOperation((uint64_t)tv, sizeof(timeval_t), SegmentDesc::SEG_W))
        return (uint64_t)-1;

    uint64_t ms = TrapHandler::getTicks();

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    tv->tv_sec  = (int64_t)(ms / 1000);
    tv->tv_usec = (int64_t)((ms % 1000) * 1000);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return 0;
}

struct rusage_t {
    timeval_t ru_utime;
    timeval_t ru_stime;
    int64_t   ru_maxrss;
    int64_t   ru_ixrss;
    int64_t   ru_idrss;
    int64_t   ru_isrss;
    int64_t   ru_minflt;
    int64_t   ru_majflt;
    int64_t   ru_nswap;
    int64_t   ru_inblock;
    int64_t   ru_oublock;
    int64_t   ru_msgsnd;
    int64_t   ru_msgrcv;
    int64_t   ru_nsignals;
    int64_t   ru_nvcsw;
    int64_t   ru_nivcsw;
};

uint64_t SyscallHandler::sys_getrusage(TrapFrame* tf) {
    int who = (int)(int64_t)tf->a0;
    auto* usage = (rusage_t*)tf->a1;

    if (!usage) return (uint64_t)-1;

    if (!PCB::runningProcess()->checkOperation((uint64_t)usage, sizeof(rusage_t), SegmentDesc::SEG_W))
        return (uint64_t)-1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    memset(usage, 0, sizeof(rusage_t));
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return 0;
}

uint64_t SyscallHandler::sys_umask(TrapFrame* tf) {
    uint32_t newmask = (uint32_t)tf->a0;
    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)022;
    return (uint64_t)proc->setUmask(newmask);
}

uint64_t SyscallHandler::sys_kill(TrapFrame* tf) {
    auto pid = (pid_t)(int64_t)tf->a0;
    auto signum = (int)(int64_t)tf->a1;

    if (signum < 0 || signum >= NSIG) return (uint64_t)-22;

    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;

    if ((int64_t)pid < -1) {
        Process::signalProcessGroup((pid_t)(-(int64_t)pid), signum);
        return 0;
    }

    if ((int64_t)pid == 0) {
        Process::signalProcessGroup(proc->pgid(), signum);
        return 0;
    }

    if (pid == (pid_t)proc->pid()) {
        return (uint64_t)proc->kill(signum);
    }

    Process* target = Process::findProcess(pid);
    if (!target) return (uint64_t)-3;

    return (uint64_t)target->kill(signum);
}

uint64_t SyscallHandler::sys_tkill(TrapFrame* tf) {
    auto signum = (int)(int64_t)tf->a1;
    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;
    return (uint64_t)proc->kill(signum);
}

uint64_t SyscallHandler::sys_tgkill(TrapFrame* tf) {
    auto signum = (int)(int64_t)tf->a2;
    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;
    return (uint64_t)proc->kill(signum);
}

uint64_t SyscallHandler::sys_rt_sigaction(TrapFrame* tf) {
    auto signum = (int)(int64_t)tf->a0;
    auto actPtr = (SignalAction*)tf->a1;
    auto oldPtr = (SignalAction*)tf->a2;

    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;

    SignalAction kact, kold;

    if (actPtr) {
        if (!proc->checkOperation((uint64_t)actPtr, sizeof(SignalAction), SegmentDesc::SEG_R))
            return (uint64_t)-14;
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        memcpy(&kact, actPtr, sizeof(SignalAction));
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }

    int ret = proc->sigaction(signum,
                              actPtr  ? &kact : nullptr,
                              oldPtr  ? &kold : nullptr);
    if (ret != 0) return (uint64_t)(int64_t)ret;

    if (oldPtr) {
        if (!proc->checkOperation((uint64_t)oldPtr, sizeof(SignalAction), SegmentDesc::SEG_W))
            return (uint64_t)-14;
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        memcpy(oldPtr, &kold, sizeof(SignalAction));
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }
    return 0;
}

uint64_t SyscallHandler::sys_rt_sigprocmask(TrapFrame* tf) {
    auto how    = (int)(int64_t)tf->a0;
    auto setPtr = (uint64_t*)tf->a1;
    auto oldPtr = (uint64_t*)tf->a2;

    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;

    uint64_t kset = 0;
    if (setPtr) {
        if (!proc->checkOperation((uint64_t)setPtr, sizeof(uint64_t), SegmentDesc::SEG_R))
            return (uint64_t)-14;
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        kset = *setPtr;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }

    uint64_t kold = 0;
    int ret = proc->sigprocmask(how, setPtr ? &kset : nullptr, &kold);
    if (ret != 0) return (uint64_t)(int64_t)ret;

    if (oldPtr) {
        if (!proc->checkOperation((uint64_t)oldPtr, sizeof(uint64_t), SegmentDesc::SEG_W))
            return (uint64_t)-14;
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        *oldPtr = kold;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }
    return 0;
}

uint64_t SyscallHandler::sys_rt_sigreturn(TrapFrame* tf) {
    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;

    uint64_t sp = tf->sp;
    if (!proc->checkOperation(sp, sizeof(SignalFrame), SegmentDesc::SEG_R))
        return (uint64_t)-14;

    SignalFrame frame;
    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    memcpy(&frame, (void*)sp, sizeof(SignalFrame));
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    tf->sepc = frame.sepc;
    tf->ra = frame.ra;
    tf->sp = frame.sp;
    tf->gp = frame.gp;
    tf->tp = frame.tp;
    tf->t0 = frame.t0;
    tf->t1 = frame.t1;
    tf->t2 = frame.t2;
    tf->s0 = frame.s0;
    tf->s1 = frame.s1;
    tf->a0 = frame.a0;
    tf->a1 = frame.a1;
    tf->a2 = frame.a2;
    tf->a3 = frame.a3;
    tf->a4 = frame.a4;
    tf->a5 = frame.a5;
    tf->a6 = frame.a6;
    tf->a7 = frame.a7;
    tf->s2 = frame.s2;
    tf->s3 = frame.s3;
    tf->s4 = frame.s4;
    tf->s5 = frame.s5;
    tf->s6 = frame.s6;
    tf->s7 = frame.s7;
    tf->s8 = frame.s8;
    tf->s9 = frame.s9;
    tf->s10 = frame.s10;
    tf->s11 = frame.s11;
    tf->t3 = frame.t3;
    tf->t4 = frame.t4;
    tf->t5 = frame.t5;
    tf->t6 = frame.t6;
    tf->kstack = frame.kstack;

    PCB::running()->m_sigMask = frame.saved_mask;

    return tf->a0;
}

uint64_t SyscallHandler::sys_rt_sigpending(TrapFrame* tf) {
    auto setPtr = (uint64_t*)tf->a0;
    Process* proc = PCB::runningProcess();
    if (!proc || !setPtr) return (uint64_t)-1;
    if (!proc->checkOperation((uint64_t)setPtr, sizeof(uint64_t), SegmentDesc::SEG_W))
        return (uint64_t)-14;

    uint64_t pending = 0;
    if (proc->m_signalHandler)
        pending = proc->m_signalHandler->hasPending(proc->m_sigMask)
                      ? proc->m_sigMask
                      : 0;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    *setPtr = pending;
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    return 0;
}

uint64_t SyscallHandler::sys_sigaltstack(TrapFrame* tf) {
    return 0;
}

uint64_t SyscallHandler::sys_madvise(TrapFrame* tf) {
    return 0;
}

uint64_t SyscallHandler::sys_prlimit64(TrapFrame* tf) {
    pid_t pid = (pid_t)(int64_t)tf->a0;
    int resource = (int)(int64_t)tf->a1;
    uint64_t new_lim = tf->a2;
    uint64_t old_lim = tf->a3;

    struct rlimit {
        uint64_t rlim_cur;
        uint64_t rlim_max;
    };

    static constexpr uint64_t RLIM_INFINITY = ~0ULL;
    static constexpr int RLIMIT_STACK = 3;
    static constexpr int RLIMIT_NOFILE = 7;
    static constexpr int RLIMIT_AS = 9;

    if (old_lim) {
        if (!PCB::runningProcess()->checkOperation(
                old_lim, sizeof(rlimit), SegmentDesc::SEG_W))
            return (uint64_t)-14;

        rlimit lim;
        switch (resource) {
            case RLIMIT_STACK:
                lim.rlim_cur = PCB::USER_STACK_SIZE;
                lim.rlim_max = PCB::USER_STACK_SIZE;
                break;
            case RLIMIT_NOFILE:
                lim.rlim_cur = FdTable::MAX_FDS;
                lim.rlim_max = FdTable::MAX_FDS;
                break;
            case RLIMIT_AS:
                lim.rlim_cur = RLIM_INFINITY;
                lim.rlim_max = RLIM_INFINITY;
                break;
            default:
                lim.rlim_cur = RLIM_INFINITY;
                lim.rlim_max = RLIM_INFINITY;
                break;
        }

        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        *((rlimit*)old_lim) = lim;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }

    return 0;
}

uint64_t SyscallHandler::sys_getrandom(TrapFrame* tf) {
    void* buf = (void*)tf->a0;
    uint64_t len = tf->a1;
    uint32_t flags = (uint32_t)tf->a2;

    if (!buf || len == 0) return (uint64_t)-22;

    if (!PCB::runningProcess()->checkOperation(
            (uint64_t)buf, len, SegmentDesc::SEG_W))
        return (uint64_t)-14;

    static uint64_t s_counter = 0;
    s_counter++;

    auto* dst = (uint8_t*)buf;
    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    for (uint64_t i = 0; i < len; i++) {
        uint64_t val = TrapHandler::getTicks();
        val ^= s_counter * 6364136223846793005ULL;
        val ^= (uint64_t)buf * 2654435761ULL;
        val ^= i * 0xff51afd7ed558ccdULL;
        val ^= (val >> 33);
        val *= 0xff51afd7ed558ccdULL;
        val ^= (val >> 33);
        val *= 0xc4ceb9fe1a85ec53ULL;
        val ^= (val >> 33);
        dst[i] = (uint8_t)(val & 0xFF);
    }
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return len;
}

uint64_t SyscallHandler::sys_ioctl(TrapFrame* tf) {
    int fd  = (int)(int64_t)tf->a0;
    uint64_t req = tf->a1;
    uint64_t argAddr = tf->a2;

    File* file = PCB::runningProcess()->getFile(fd);
    if (!file) return (uint64_t)-9; // EBADF

    switch (req) {
        case TCGETS: {
            if (!PCB::runningProcess()->checkOperation(argAddr, sizeof(ktermios), SegmentDesc::SEG_W))
                return (uint64_t)-14; // EFAULT

            ktermios kt;
            int r = file->ioctl(req, &kt);
            if (r < 0) return (uint64_t)-25; // ENOTTY

            RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
            memcpy((void*)argAddr, &kt, sizeof(kt));
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return 0;
        }

        case TCSETS:
        case TCSETSW:
        case TCSETSF: {
            if (!PCB::runningProcess()->checkOperation(argAddr, sizeof(ktermios), SegmentDesc::SEG_R))
                return (uint64_t)-14;

            ktermios kt;
            RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
            memcpy(&kt, (void*)argAddr, sizeof(kt));
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

            int r = file->ioctl(req, &kt);
            return r < 0 ? (uint64_t)-25 : 0;
        }

        case TIOCGWINSZ: {
            if (!PCB::runningProcess()->checkOperation(argAddr, sizeof(kwinsize), SegmentDesc::SEG_W))
                return (uint64_t)-14;

            kwinsize ws;
            int r = file->ioctl(req, &ws);
            if (r < 0) return (uint64_t)-25;

            RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
            memcpy((void*)argAddr, &ws, sizeof(ws));
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return 0;
        }

        case TIOCGPGRP: {
            if (!PCB::runningProcess()->checkOperation(argAddr, sizeof(int), SegmentDesc::SEG_W))
                return (uint64_t)-14;

            int pgrp;
            int r = file->ioctl(req, &pgrp);
            if (r < 0) return (uint64_t)-25;

            RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
            memcpy((void*)argAddr, &pgrp, sizeof(pgrp));
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return 0;
        }

        case TIOCSPGRP: {
            if (!PCB::runningProcess()->checkOperation(argAddr, sizeof(int), SegmentDesc::SEG_R))
                return (uint64_t)-14;

            int pgrp;
            RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
            memcpy(&pgrp, (void*)argAddr, sizeof(pgrp));
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

            int r = file->ioctl(req, &pgrp);
            return r < 0 ? (uint64_t)-25 : 0;
        }

        case FIONREAD:
            return (uint64_t)-25;

        default:
            return (uint64_t)-25;
    }
}

uint64_t SyscallHandler::sys_statx(TrapFrame* tf) {
    int dirfd = (int)(int64_t)tf->a0;
    uint64_t pathAddr = tf->a1;
    int flags = (int)(int64_t)tf->a2;
    uint32_t mask = (uint32_t)tf->a3;
    uint64_t statxbuf = tf->a4;

    struct statx_t {
        uint32_t stx_mask;
        uint32_t stx_blksize;
        uint64_t stx_attributes;
        uint32_t stx_nlink;
        uint32_t stx_uid;
        uint32_t stx_gid;
        uint16_t stx_mode;
        uint16_t _pad1[3];
        uint64_t stx_ino;
        uint64_t stx_size;
        uint64_t stx_blocks;
        uint64_t stx_attributes_mask;
        struct { int64_t sec; uint32_t nsec; uint32_t pad; } stx_atime;
        struct { int64_t sec; uint32_t nsec; uint32_t pad; } stx_btime;
        struct { int64_t sec; uint32_t nsec; uint32_t pad; } stx_ctime;
        struct { int64_t sec; uint32_t nsec; uint32_t pad; } stx_mtime;
        uint32_t stx_rdev_major;
        uint32_t stx_rdev_minor;
        uint32_t stx_dev_major;
        uint32_t stx_dev_minor;
        uint64_t _spare[14];
    };

    if (!statxbuf) return (uint64_t)-22;
    if (!PCB::runningProcess()->checkOperation(
            statxbuf, sizeof(statx_t), SegmentDesc::SEG_W))
        return (uint64_t)-14;

    VfsInode* inode = nullptr;

    static constexpr int AT_EMPTY_PATH_LOCAL = 0x1000;
    if (flags & AT_EMPTY_PATH_LOCAL) {
        if (dirfd >= 0) {
            File* f = PCB::runningProcess()->getFile(dirfd);
            if (!f) return (uint64_t)-9;
            InodeStat ist;
            f->fstat(&ist);

            RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
            auto* sx = (statx_t*)statxbuf;
            memset(sx, 0, sizeof(statx_t));
            sx->stx_mask = mask;
            sx->stx_ino = ist.st_ino;
            sx->stx_mode = (uint16_t)ist.st_mode;
            sx->stx_nlink = ist.st_nlink;
            sx->stx_uid = ist.st_uid;
            sx->stx_gid = ist.st_gid;
            sx->stx_size = (uint64_t)ist.st_size;
            sx->stx_blksize = ist.st_blksize;
            sx->stx_blocks = (uint64_t)ist.st_blocks;
            RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
            return 0;
        }
    }

    AutoPath path(copyPathFromUser(pathAddr));
    if (!path.valid()) return (uint64_t)-14;

    inode = VFS::resolvePath(path);
    if (!inode) return (uint64_t)-2;

    InodeStat ist;
    inode->stat(&ist);
    uint32_t inum = inode->inodeNum();
    VFS::putInode(inode, inum);

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    auto* sx = (statx_t*)statxbuf;
    memset(sx, 0, sizeof(statx_t));
    sx->stx_mask = mask;
    sx->stx_ino = ist.st_ino;
    sx->stx_mode = (uint16_t)ist.st_mode;
    sx->stx_nlink = ist.st_nlink;
    sx->stx_uid = ist.st_uid;
    sx->stx_gid = ist.st_gid;
    sx->stx_size = (uint64_t)ist.st_size;
    sx->stx_blksize = ist.st_blksize;
    sx->stx_blocks = (uint64_t)ist.st_blocks;
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return 0;
}

uint64_t SyscallHandler::sys_pipe2(TrapFrame* tf) {
    auto* fds = (int*)tf->a0;

    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;

    if (!proc->checkOperation((uint64_t)fds, sizeof(int) * 2, SegmentDesc::SEG_W))
        return (uint64_t)-14; // EFAULT

    auto* pipeInode = new PipeInode();
    if (!pipeInode) return (uint64_t)-12; // ENOMEM

    auto* readFile  = new File(pipeInode, nullptr, File::O_RDONLY);
    auto* writeFile = new File(pipeInode, nullptr, File::O_WRONLY);

    int rfd = proc->m_fdTable->alloc(readFile);
    if (rfd < 0) {
        readFile->close();  delete readFile;
        writeFile->close(); delete writeFile;
        return (uint64_t)-24; // EMFILE
    }

    int wfd = proc->m_fdTable->alloc(writeFile);
    if (wfd < 0) {
        proc->m_fdTable->close(rfd);
        writeFile->close(); delete writeFile;
        return (uint64_t)-24;
    }

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    fds[0] = rfd;
    fds[1] = wfd;
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return 0;
}

uint64_t SyscallHandler::sys_setpgid(TrapFrame* tf) {
    auto pid = (pid_t)(int64_t)tf->a0;
    auto pgid = (pid_t)(int64_t)tf->a1;

    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;

    return (uint64_t)(int64_t)proc->setpgid(pid, pgid);
}

uint64_t SyscallHandler::sys_getpgid(TrapFrame* tf) {
    auto pid = (pid_t)(int64_t)tf->a0;

    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;

    return (uint64_t)(int64_t)proc->getpgid(pid);
}

uint64_t SyscallHandler::sys_setsid(TrapFrame* tf) {
    Process* proc = PCB::runningProcess();
    if (!proc) return (uint64_t)-1;

    return (uint64_t)(int64_t)proc->setsid();
}