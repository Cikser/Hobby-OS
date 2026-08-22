#include "process.h"
#include "../scheduler.h"
#include "garbage.h"
#include "../../fs/path_utils.h"
#include "../../hw/riscv.h"
#include "../thread/thread.h"
#include "../../io/console/uart_inode.h"
#include "../../fs/vfs.h"
#include "../../io/console/console.h"
#include "../../mm/mem.h"
#include "../../mm/vm/vm.h"
#include "../elf/elf.h"
#include "../../io/terminal/tty_inode.h"

KMemCache<Process>* Process::s_cache = nullptr;
Process* Process::s_init = nullptr;
Lock Process::s_allLock = Lock();
Process* Process::s_allHead = nullptr;

Process::Process(PMT* pmt, uint64_t entry, Process* parent, FdTable* fdTable) :
    PCB(entry, pmt),
    m_threads(nullptr),
    m_parent(parent),
    m_nextSibling(nullptr),
    m_firstChild(nullptr),
    m_exitCode(0),
    m_termSignal(0),
    m_selfSem(Semaphore(0)),
    m_spaceLock(Lock()),
    m_mmap(nullptr),
    m_fdTable(nullptr),
    m_pgid(0),
    m_sid(0),
    m_allNext(nullptr)
{
    m_tgid = m_pid;
    if (parent) {
        m_fdTable = fdTable ? fdTable : parent->m_fdTable->clone();
        m_cwdInode = parent->m_cwdInode;
        m_cwdPath  = kstrdup(parent->m_cwdPath, PATH_MAX);
        m_pgid = parent->m_pgid;
        m_sid = parent->m_sid;
        m_umask = parent->m_umask;
    }
    else {
        uint64_t ustackPa = MemoryLayout::v2p((uint64_t)m_ustack);
        m_pmt->mapPages(
            USER_STACK_TOP - USER_STACK_SIZE,
            ustackPa,
            USER_STACK_SIZE / MemoryLayout::PAGE_SIZE,
            PMT::PAGE_USER
        );
        m_segTable = new SegmentTable();
        m_segTable->setStack(SegmentDesc::SEG_R | SegmentDesc::SEG_W,
            USER_STACK_TOP - USER_STACK_SIZE, USER_STACK_TOP);
        m_cwdInode = 2;
        m_cwdPath  = kstrdup("/", PATH_MAX);
        m_pgid = m_pid;
        m_sid  = m_pid;

        m_fdTable = new FdTable();
        TTYInode* terminal = new TTYInode();
        terminal->setForegroundPgid(m_pgid);
        m_fdTable->alloc(new File(terminal, nullptr, File::O_RDONLY));
        m_fdTable->alloc(new File(terminal, nullptr, File::O_WRONLY));
        m_fdTable->alloc(new File(terminal, nullptr, File::O_WRONLY));

        m_mmap = new Mmap(m_pmt, m_segTable);
        m_signalHandler = new SignalHandler();
        m_sigMask = 0;
    }

    registerProcess(this);
}

void Process::clear() {
    unregisterProcess(this);

    Thread* t = m_threads;
    while (t) {
        Thread* next = t->m_nextThread;
        t->exit(0);
        t = next;
    }
    if (m_fdTable) {
        m_fdTable->release();
        m_fdTable = nullptr;
    }
    delete m_mmap;
    delete m_segTable;
    if (m_cwdPath) {
        MemoryAllocator::kfree(m_cwdPath);
        m_cwdPath = nullptr;
    }
    VM::destroyPMT(m_pmt);
    /*if (m_signalHandler && m_signalHandler->release()) {
        delete m_signalHandler;
        m_signalHandler = nullptr;
    }*/
    PCB::clear();
}

uint64_t Process::setupInitialStack(const char* path, const ElfLoadInfo& elfInfo, uint8_t* randomBytes16,
                                    char* argv[], char* envp[]) const {
    uint8_t* stackTop = m_ustack + USER_STACK_SIZE;
    uint8_t* p = stackTop;

    auto push8 = [&](uint64_t val) {
        p -= 8;
        *(uint64_t*)p = val;
    };

    auto getVirtAddr = [&](uint8_t* ptr) -> uint64_t {
        return USER_STACK_TOP - (stackTop - ptr);
    };

    int32_t argc = 0;
    if (argv) {
        while (argv[argc]) argc++;
    }

    int32_t envc = 0;
    if (envp) {
        while (envp[envc]) envc++;
    }

    uint64_t argv_addrs[argc > 0 ? argc : 1];
    uint64_t envp_addrs[envc > 0 ? envc : 1];

    if (argc == 0) {
        size_t len = strlen(path) + 1;
        p -= len;
        memcpy(p, path, len);
        argv_addrs[0] = getVirtAddr(p);
        argc = 1;
    }
    else {
        for (int32_t i = envc - 1; i >= 0; i--) {
            size_t len = strlen(envp[i]) + 1;
            p -= len;
            memcpy(p, envp[i], len);
            envp_addrs[i] = getVirtAddr(p);
        }

        for (int i = argc - 1; i >= 0; i--) {
            size_t len = strlen(argv[i]) + 1;
            p -= len;
            memcpy(p, argv[i], len);
            argv_addrs[i] = getVirtAddr(p);
        }
    }

    p = (uint8_t*)((uint64_t)p & ~0xFULL);

    p -= 16;
    memcpy(p, randomBytes16, 16);
    uint64_t at_random_addr = getVirtAddr(p);
    
    size_t wordsCount = (15 * 2) + (envc + 1) + (argc + 1) + 1;

    uint64_t targetSp = (uint64_t)p - (wordsCount * 8);
    if (targetSp % 16 != 0) {
        p -= 8;
    }

    push8(0); push8(0);
    push8(0); push8(23);
    push8(0); push8(14);
    push8(0); push8(13);
    push8(0); push8(12);
    push8(0); push8(11);
    push8(elfInfo.entry); push8(9);
    push8(at_random_addr); push8(25);
    push8(0); push8(16);
    push8(4096); push8(6);
    push8(elfInfo.phnum); push8(5);
    push8(elfInfo.phent); push8(4);
    push8(elfInfo.phdr_va); push8(3);

    push8(0);
    for (int i = envc - 1; i >= 0; i--) {
        push8(envp_addrs[i]);
    }

    push8(0);
    for (int i = argc - 1; i >= 0; i--) {
        push8(argv_addrs[i]);
    }

    push8(argc);

    uint64_t sp_offset = stackTop - p;
    return USER_STACK_TOP - sp_offset;
}

Process* Process::findProcess(pid_t pid) {
    return findByPid(pid);
}

Process* Process::createInit() {
    PMT* pmt = VM::createPMT();
    auto proc = new Process(pmt, -1, nullptr);

    ElfLoadInfo* info = ElfLoader::load("/bin/init", pmt, proc->m_segTable);
    if (!info || !info->entry)
        Console::panic("Process::createInit(): failed to load ELF");

    uint8_t randomBytes[16] = {0};

    uint64_t initialSp = proc->setupInitialStack("/bin/init", *info, randomBytes, nullptr, nullptr);

    proc->m_entry = info->entry;
    proc->m_trapFrame->sepc = info->entry;
    proc->m_trapFrame->sp = initialSp;
    proc->m_entry = info->entry;
    proc->m_segTable->setHeap(SegmentDesc::SEG_R | SegmentDesc::SEG_W, HEAP_START, HEAP_START);
    s_init = proc;
    MemoryAllocator::kfree(info);
    return proc;
}

Thread* Process::createThread(void (*entry)(void*), void* args) {
    auto t = new Thread(this, (uint64_t)entry, args);

    m_lock.acquire();
    t->m_nextThread = m_threads;
    m_threads = t;
    m_lock.release();

    return t;
}

Process* Process::fork() {
    m_spaceLock.acquire();

    PMT* pmt = VM::createPMT();
    if (!pmt) {
        m_spaceLock.release();
        return nullptr;
    }

    if (!VM::copyPMT(pmt, m_pmt, m_mmap)) {
        delete pmt;
        m_spaceLock.release();
        return nullptr;
    }

    FdTable* childFds = m_fdTable->clone();
    auto child = new Process(pmt, -1, this, childFds);
    child->m_signalHandler = m_signalHandler->clone();
    child->m_sigMask = m_sigMask;

    memcpy(child->m_trapFrame, m_trapFrame, sizeof(TrapFrame));
    child->m_entry = m_trapFrame->sepc;
    child->m_trapFrame->a0 = 0;
    child->m_trapFrame->kstack = (uint64_t)child->m_kstack + KERNEL_STACK_SIZE;

    child->m_segTable = SegmentTable::copy(m_segTable);
    child->m_cwdInode = m_cwdInode;
    strcpy(child->m_cwdPath, m_cwdPath);
    delete child->m_mmap;
    child->m_mmap = m_mmap
        ? m_mmap->clone(child->m_pmt, child->m_segTable)
        : new Mmap(child->m_pmt, child->m_segTable);
    m_spaceLock.release();
    m_lock.acquire();
    child->m_nextSibling = m_firstChild;
    m_firstChild = child;
    m_lock.release();
    return child;
}

File* Process::getFile(int fd) const {
    return m_fdTable ? m_fdTable->get(fd) : nullptr;
}

int Process::exec(const char* elfPath, char* argv[], char* envp[]) {
    m_spaceLock.acquire();
    File* elf = VFS::open(elfPath, File::O_RDONLY);
    if (!elf) {
        m_spaceLock.release();
        return -1;
    }
    elf->close();
    delete elf;

    uint8_t* newUstack = (uint8_t*)MemoryAllocator::kallocPages(USER_STACK_SIZE / MemoryLayout::PAGE_SIZE);
    if (!newUstack) {
        m_spaceLock.release();
        return -1;
    }

    VM::clearUserPages(m_pmt);
    m_segTable->clear();

    ElfLoadInfo* info = ElfLoader::load(elfPath, m_pmt, m_segTable);
    if (!info) {
        MemoryAllocator::kfreePages(newUstack, USER_STACK_SIZE / MemoryLayout::PAGE_SIZE);
        m_spaceLock.release();
        return -1;
    }

    m_ustack = newUstack;
    uint64_t ustackPa = MemoryLayout::v2p((uint64_t)m_ustack);
    m_pmt->mapPages(
            USER_STACK_TOP - USER_STACK_SIZE,
            ustackPa,
            USER_STACK_SIZE / MemoryLayout::PAGE_SIZE,
            PMT::PAGE_USER
        );

    uint8_t randomBytes[16] = {0};
    uint64_t initialSp = setupInitialStack(elfPath, *info, randomBytes, argv, envp);

    uint64_t oldKstack = m_trapFrame->kstack;
    memset(m_trapFrame, 0, sizeof(TrapFrame));
    m_trapFrame->kstack = oldKstack;
    m_trapFrame->sepc = info->entry;
    m_trapFrame->sp = initialSp;

    m_entry = info->entry;
    MemoryAllocator::kfree(info);

    m_segTable->setHeap(SegmentDesc::SEG_R | SegmentDesc::SEG_W, HEAP_START,HEAP_START);
    m_segTable->setStack(SegmentDesc::SEG_R | SegmentDesc::SEG_W,
        USER_STACK_TOP - USER_STACK_SIZE, USER_STACK_TOP);

    RiscV::flushTLB();
    m_spaceLock.release();
    return 0;
}

uint64_t Process::brk(uint64_t newHeapEnd) const {
    m_spaceLock.acquire();

    uint64_t heapStart = m_segTable->heap()->start;
    uint64_t heapEnd = m_segTable->heap()->end;

    if (newHeapEnd == 0 || newHeapEnd < heapStart || newHeapEnd == heapEnd) {
        m_spaceLock.release();
        return heapEnd;
    }

    if (newHeapEnd > heapEnd + MemoryAllocator::freePages() * MemoryLayout::PAGE_SIZE) {
        m_spaceLock.release();
        return -1;
    }

    if (newHeapEnd > heapEnd) {
        uint64_t alignedNew = MemoryLayout::pageRoundUp(newHeapEnd);
        uint64_t alignedOld = MemoryLayout::pageRoundUp(heapEnd);
        uint32_t pageNum = (alignedNew - alignedOld) / MemoryLayout::PAGE_SIZE;

        for (uint32_t i = 0; i < pageNum; i++) {
            auto page = (uint64_t)MemoryAllocator::kallocPage();
            if (page == 0) {
                m_spaceLock.release();
                return -1;
            }
            uint64_t pagePa = MemoryLayout::v2p(page);
            if (m_pmt->mapPage(alignedOld + i * MemoryLayout::PAGE_SIZE,
                               pagePa, PMT::PAGE_USER)) {
                heapEnd = alignedOld + (i + 1) * MemoryLayout::PAGE_SIZE;
            }
            else {
                m_spaceLock.release();
                return -1;
            }
        }
    }
    else {
        uint64_t alignedNew = MemoryLayout::pageRoundUp(newHeapEnd);
        uint64_t alignedOld = MemoryLayout::pageRoundUp(heapEnd);
        uint32_t pageNum = (alignedOld - alignedNew) / MemoryLayout::PAGE_SIZE;

        for (uint32_t i = 0; i < pageNum; i++) {
            uint64_t va = alignedOld - (i + 1) * MemoryLayout::PAGE_SIZE;
            uint64_t pagePa = m_pmt->translate(va);
            if (m_pmt->unmapPage(va)) {
                MemoryAllocator::kfreePage((void*)MemoryLayout::p2v(pagePa));
                heapEnd = va;
            }
            else {
                m_spaceLock.release();
                return -1;
            }
        }
        m_pmt->clean();
        RiscV::flushTLB();
    }
    m_segTable->heap()->end = heapEnd;

    m_spaceLock.release();
    return heapEnd;
}

uint64_t Process::openFile(const char* path, uint64_t flags) const {
    File* file = VFS::open(path, flags);
    if (!file) return -1;

    int fd = m_fdTable->alloc(file);
    if (fd < 0) {
        file->close();
        delete file;
        return -1;
    }
    return fd;
}

int Process::closeFile(int fd) const {
    return m_fdTable ? m_fdTable->close(fd) : -1;
}

void Process::exit(int exitCode) {
    m_exitCode = exitCode;
    m_lock.acquire();
    Process* child = m_firstChild;
    while (child) {
        Process* next = child->m_nextSibling;
        child->m_parent = s_init;
        s_init->m_lock.acquire();
        child->m_nextSibling = Process::s_init->m_firstChild;
        s_init->m_firstChild = child;
        if (child->m_state == ProcState::ZOMBIE)
            s_init->m_selfSem.signal();
        s_init->m_lock.release();
        child = next;
    }
    m_firstChild = nullptr;
    if (m_tidAddress) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        *(int*)m_tidAddress = 0;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }
    while (m_waitSem.waiting()) {
        m_waitSem.signal();
    }

    if (m_parent) {
        m_parent->m_selfSem.signal();
    }
    if (m_parent)
        m_parent->kill(SIGCHLD);
    m_state = ProcState::ZOMBIE;
    clear();
    PCBGarbage::put(this);
    m_lock.release();
    yield();
}

void Process::exitViaSignal(int signum) {
    m_termSignal = signum;
    exit(0);
}

pid_t Process::wait(pid_t pid, int* status, int options) {
    m_lock.acquire();
    if (!m_firstChild) {
        m_lock.release();
        return -1;
    }

    if (pid != -1) {
        bool found = false;
        for (Process* c = m_firstChild; c; c = c->m_nextSibling) {
            if (c->m_pid == pid) { found = true; break; }
        }
        if (!found) {
            m_lock.release();
            return -1;
        }
    }
    m_lock.release();

    if (options & WNOHANG) {
        m_lock.acquire();
        bool hasZombie = false;
        for (Process* c = m_firstChild; c; c = c->m_nextSibling) {
            if (c->m_state == ProcState::ZOMBIE &&
                (pid == -1 || c->m_pid == pid)) {
                hasZombie = true;
                break;
            }
        }
        m_lock.release();
        if (!hasZombie) return 0;
    }

    m_selfSem.wait();

    m_lock.acquire();
    Process* zombie = nullptr;
    Process* prev = nullptr;
    for (Process* c = m_firstChild; c; c = c->m_nextSibling) {
        if (c->m_state == ProcState::ZOMBIE &&
            (pid == -1 || c->m_pid == pid)) {
            zombie = c;
            break;
        }
        prev = c;
    }

    if (!zombie) {
        m_lock.release();
        return -1;
    }

    if (prev)
        prev->m_nextSibling = zombie->m_nextSibling;
    else
        m_firstChild = zombie->m_nextSibling;

    m_lock.release();

    pid_t retPid = zombie->m_pid;
    if (status) {
        if (zombie->m_termSignal != 0)
            *status = zombie->m_termSignal & 0x7f;
        else
            *status = (zombie->m_exitCode & 0xff) << 8;
    }

    zombie->m_reaped = true;
    return retPid;
}

uint64_t Process::mmap(uint64_t addr, uint64_t length,
                       uint32_t prot, uint32_t flags,
                       int fd, uint64_t offset) const {
    if (!m_mmap) return (uint64_t)-1;
    m_spaceLock.acquire();
    uint64_t ret = m_mmap->map(addr, length, prot, flags, fd, offset);
    m_spaceLock.release();
    return ret;
}

int Process::munmap(uint64_t addr, uint64_t length) const {
    if (!m_mmap) return -1;
    m_spaceLock.acquire();
    int ret = m_mmap->unmap(addr, length);
    m_spaceLock.release();
    return ret;
}

int Process::mprotect(uint64_t addr, uint64_t length, uint32_t prot) const {
    if (!m_mmap) return -1;
    m_spaceLock.acquire();
    int ret = m_mmap->protect(addr, length, prot);
    m_spaceLock.release();
    return ret;
}

char* Process::resolveRelative(const char* path) const {
    char* temp = (char*)MemoryAllocator::kmalloc(PATH_MAX + 1);
    if (!temp) return nullptr;

    uint64_t len = 0;

    if (path[0] == '/') {
        temp[0] = '/';
        temp[1] = '\0';
        len = 1;
    }
    else {
        uint64_t cwdLen = strlen(m_cwdPath);
        if (cwdLen > PATH_MAX) {
            MemoryAllocator::kfree(temp);
            return nullptr;
        }
        memcpy(temp, m_cwdPath, cwdLen + 1);
        len = cwdLen;
    }

    char* component = (char*)MemoryAllocator::kmalloc(PATH_MAX + 1);
    if (!component) {
        MemoryAllocator::kfree(temp);
        return nullptr;
    }

    const char* p = (path[0] == '/') ? path + 1 : path;

    while (*p != '\0') {
        uint64_t cLen = 0;
        while (*p != '\0' && *p != '/' && cLen < PATH_MAX)
            component[cLen++] = *p++;
        component[cLen] = '\0';
        if (*p == '/') p++;
        if (cLen == 0) continue;

        if (strcmp(component, ".") == 0) {
            continue;
        }
        if (strcmp(component, "..") == 0) {
            if (len > 1) {
                len--;
                while (len > 0 && temp[len] != '/')
                    len--;
                if (len == 0) len = 1;
                temp[len] = '\0';
            }
        }
        else {
            if ((len > 1 && temp[len-1] != '/') || (len == 1 && temp[0] != '/'))
                temp[len++] = '/';

            if (len + cLen <= PATH_MAX) {
                memcpy(temp + len, component, cLen);
                len += cLen;
                temp[len] = '\0';
            } else {
                MemoryAllocator::kfree(component);
                MemoryAllocator::kfree(temp);
                return nullptr;
            }
        }
    }

    MemoryAllocator::kfree(component);

    if (len == 0) {
        temp[0] = '/';
        temp[1] = '\0';
    }

    char* result = kstrdup(temp, PATH_MAX);
    MemoryAllocator::kfree(temp);
    return result;
}

char* Process::cwd() const {
    return kstrdup(m_cwdPath ? m_cwdPath : "/", PATH_MAX);
}

Thread* Process::cloneThread(uint64_t entry, uint64_t userStack, uint64_t tls,
    int* parentTidPtr, int* childTidPtr, int* clearTidPtr) {
    auto* t = new Thread(this, entry, userStack, tls, childTidPtr, clearTidPtr);
    if (!t) return nullptr;

    if (parentTidPtr) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        *parentTidPtr = (int)t->pid();
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }

    m_fdTable->acquire();

    m_lock.acquire();
    t->m_nextThread = m_threads;
    m_threads = t;
    m_lock.release();

    return t;
}

int Process::chdir(const char* path) {
    if (!path || path[0] == '\0') return -1;

    char* resolved = resolveRelative(path);
    if (!resolved) return -1;

    File* f = VFS::open(resolved, File::O_RDONLY);
    if (!f) {
        MemoryAllocator::kfree(resolved);
        return -1;
    }
    f->close();
    delete f;

    VfsInode* inode = VFS::resolvePath(resolved);
    if (!inode) {
        MemoryAllocator::kfree(resolved);
        return -1;
    }

    if (!inode->isDir()) {
        VFS::putInode(inode, inode->inodeNum());
        MemoryAllocator::kfree(resolved);
        return -1;
    }

    uint32_t newInodeNum = inode->inodeNum();
    VFS::putInode(inode, newInodeNum);

    m_cwdInode = newInodeNum;
    uint64_t rlen = strlen(resolved);
    if (rlen > 1 && resolved[rlen - 1] == '/')
        resolved[rlen - 1] = '\0';

    if (m_cwdPath) MemoryAllocator::kfree(m_cwdPath);
    m_cwdPath = resolved;

    return 0;
}

int Process::mkdir(const char* path, uint32_t mode) const {
    if (!path || path[0] == '\0') return -1;

    char* resolved = resolveRelative(path);
    if (!resolved) return -1;

    int ret = VFS::mkdir(resolved);
    MemoryAllocator::kfree(resolved);
    return ret;
}

int Process::fstat(int fd, InodeStat* st) const {
    File* f = getFile(fd);
    if (!f) return -1;
    return f->fstat(st);
}

void Process::exitGroup(int exitCode) {
    m_lock.acquire();
    Thread* t = m_threads;
    while (t) {
        Thread* next = t->m_nextThread;
        if (t->m_state != ProcState::ZOMBIE)
            t->exit(0);
        t = next;
    }
    m_lock.release();

    exit(exitCode);
}

int Process::kill(int signum) {
    if (signum < 0 || signum >= NSIG) return -1;
    if (!m_signalHandler) return -1;

    if (signum == 0) return 0;

    m_signalHandler->send(signum);

    if (signum == SIGCONT) {
        wakeStoppedThreads();
    }

    m_lock.acquire();
    if (m_state == ProcState::SLEEPING || m_state == ProcState::BLOCKED) {
        setState(ProcState::READY);
        Scheduler::put(this);
    }
    Thread* t = m_threads;
    while (t) {
        if (t->m_state == ProcState::SLEEPING || t->m_state == ProcState::BLOCKED) {
            t->setState(ProcState::READY);
            Scheduler::put(t);
        }
        t = t->m_nextThread;
    }
    m_lock.release();
    return 0;
}

int Process::sigaction(int signum, const SignalAction* act, SignalAction* oldact) const {
    if (!m_signalHandler) return -1;
    return m_signalHandler->setAction(signum, act, oldact);
}

int Process::sigprocmask(int how, const uint64_t* set, uint64_t* oldset) {
    if (oldset) *oldset = m_sigMask;
    if (!set)   return 0;

    constexpr int SIG_BLOCK = 0;
    constexpr int SIG_UNBLOCK = 1;
    constexpr int SIG_SETMASK = 2;

    uint64_t newMask = m_sigMask;
    switch (how) {
    case SIG_BLOCK: newMask |= *set; break;
    case SIG_UNBLOCK: newMask &= ~(*set); break;
    case SIG_SETMASK: newMask  = *set; break;
    default: return -1;
    }
    newMask &= ~(sigBit(SIGKILL) | sigBit(SIGSTOP));
    m_sigMask = newMask;
    return 0;
}

void Process::registerProcess(Process* p) {
    s_allLock.acquire();
    p->m_allNext = s_allHead;
    s_allHead = p;
    s_allLock.release();
}

void Process::unregisterProcess(Process* p) {
    s_allLock.acquire();
    Process* cur = s_allHead, *prev = nullptr;
    while (cur) {
        if (cur == p) {
            if (prev) prev->m_allNext = cur->m_allNext;
            else s_allHead = cur->m_allNext;
            break;
        }
        prev = cur;
        cur = cur->m_allNext;
    }
    s_allLock.release();
}

Process* Process::findByPid(pid_t pid) {
    s_allLock.acquire();
    Process* p = s_allHead;
    while (p) {
        if (p->m_pid == pid) {
            s_allLock.release();
            return p;
        }
        p = p->m_allNext;
    }
    s_allLock.release();
    return nullptr;
}

void Process::signalProcessGroup(pid_t pgid, int signum) {
    if (pgid <= 0) return;
    s_allLock.acquire();
    for (Process* p = s_allHead; p; p = p->m_allNext) {
        if (p->m_pgid == pgid) {
            p->kill(signum);
        }
    }
    s_allLock.release();
}

int Process::setpgid(pid_t targetPid, pid_t pgid) {
    Process* target = (targetPid == 0) ? this : findByPid(targetPid);
    if (!target) return -1;

    pid_t newPgid = (pgid == 0) ? target->m_pid : pgid;
    if ((int64_t)newPgid < 0) return -1;

    if (target->m_sid != m_sid) return -1;

    target->m_pgid = newPgid;
    return 0;
}

pid_t Process::getpgid(pid_t targetPid) const {
    if (targetPid == 0) return m_pgid;
    Process* target = findByPid(targetPid);
    if (!target) return (pid_t)-1;
    return target->m_pgid;
}

pid_t Process::setsid() {
    if (m_pgid == m_pid) return (pid_t)-1;
    m_sid = m_pid;
    m_pgid = m_pid;
    return m_pid;
}

void Process::notifyStopped(int signum) {
    if (m_parent) m_parent->kill(SIGCHLD);
}

void Process::wakeStoppedThreads() {
    m_lock.acquire();
    if (m_state == ProcState::STOPPED) {
        setState(ProcState::READY);
        Scheduler::put(this);
    }
    Thread* t = m_threads;
    while (t) {
        if (t->state() == ProcState::STOPPED) {
            t->setState(ProcState::READY);
            Scheduler::put(t);
        }
        t = t->m_nextThread;
    }
    m_lock.release();
}

uint32_t Process::setUmask(uint32_t mask) {
    uint32_t old = m_umask;
    m_umask = mask & 0777;
    return old;
}