#include "process.h"

#include "garbage.h"
#include "path_utils.h"
#include "riscv.h"
#include "../thread/thread.h"
#include "../../io/console/uart_inode.h"
#include "../../fs/vfs.h"
#include "../../io/console/console.h"
#include "../../mm/mem.h"
#include "../../mm/vm/vm.h"
#include "../elf/elf.h"

KMemCache<Process>* Process::s_cache = nullptr;
Process* Process::s_init = nullptr;

Process::Process(PMT* pmt, uint64_t entry, Process* parent, FdTable* fdTable) :
    PCB(entry, pmt),
    m_threads(nullptr),
    m_parent(parent),
    m_nextSibling(nullptr),
    m_firstChild(nullptr),
    m_exitCode(0),
    m_selfSem(Semaphore(0)),
    m_spaceLock(Lock()),
    m_mmap(nullptr),
    m_fdTable(nullptr)
{
    m_tgid = m_pid;
    if (parent) {
        m_fdTable = fdTable ? fdTable : parent->m_fdTable->clone();
        m_cwdInode = parent->m_cwdInode;
        m_cwdPath  = kstrdup(parent->m_cwdPath, PATH_MAX);
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

        m_fdTable = new FdTable();
        m_fdTable->alloc(new File(UartInode::instance(), nullptr, File::O_RDONLY));
        m_fdTable->alloc(new File(UartInode::instance(), nullptr, File::O_WRONLY));
        m_fdTable->alloc(new File(UartInode::instance(), nullptr, File::O_WRONLY));

        m_mmap = new Mmap(m_pmt, m_segTable);
    }
}

void Process::clear() {
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
    delete m_cwdPath;
    VM::destroyPMT(m_pmt);
    PCB::clear();
}

Process* Process::createInit() {
    PMT* pmt = VM::createPMT();
    auto proc = new Process(pmt, -1, nullptr);

    uint64_t entry = ElfLoader::load("/bin/init", pmt, proc->m_segTable);
    if (!entry)
        Console::panic("Process::createInit(): failed to load ELF");

    proc->m_entry = entry;
    proc->m_segTable->setHeap(SegmentDesc::SEG_R | SegmentDesc::SEG_W, HEAP_START, HEAP_START);
    s_init = proc;
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
    uint64_t childStackVa = USER_STACK_TOP -
        child->m_pid * (USER_STACK_SIZE + MemoryLayout::PAGE_SIZE)
        - USER_STACK_SIZE;

    uint64_t constructorPa = child->m_pmt->translate(childStackVa);
    if (constructorPa) {
        child->m_pmt->unmapPage(childStackVa);
        MemoryAllocator::kfreePage((void*)MemoryLayout::p2v(constructorPa));
    }

    uint64_t parentStackVa = USER_STACK_TOP -
        m_pid * (USER_STACK_SIZE + MemoryLayout::PAGE_SIZE)
        - USER_STACK_SIZE;

    uint64_t sharedPa = pmt->translate(parentStackVa);
    child->m_ustack = sharedPa
        ? (uint8_t*)MemoryLayout::p2v(sharedPa)
        : m_ustack;

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

int Process::exec(const char* elfPath) {
    m_spaceLock.acquire();
    File* elf = VFS::open(elfPath, File::O_RDONLY);
    if (!elf) {
        m_spaceLock.release();
        return -1;
    }
    elf->close();
    m_ustack = (uint8_t*)MemoryAllocator::kallocPages(USER_STACK_SIZE / MemoryLayout::PAGE_SIZE);
    if (!m_ustack) {
        m_spaceLock.release();
        return -1;
    }
    uint64_t ustackPa = MemoryLayout::v2p((uint64_t)m_ustack);
    VM::clearUserPages(m_pmt);
    m_segTable->clear();
    uint64_t entry = ElfLoader::load(elfPath, m_pmt, m_segTable);
    if (!entry) {
        m_spaceLock.release();
        return -1;
    }
    uint64_t oldKstack = m_trapFrame->kstack;
    memset(m_trapFrame, 0, sizeof(TrapFrame));
    m_trapFrame->kstack = oldKstack;
    m_trapFrame->sepc = entry;
    m_trapFrame->sp = USER_STACK_TOP;
    m_pmt->mapPages(
            USER_STACK_TOP - USER_STACK_SIZE,
            ustackPa,
            USER_STACK_SIZE / MemoryLayout::PAGE_SIZE,
            PMT::PAGE_USER
        );
    m_entry = entry;
    m_trapFrame->sepc = entry;
    m_trapFrame->sp = USER_STACK_TOP;
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
    m_state = ProcState::ZOMBIE;
    clear();
    PCBGarbage::put(this);
    m_lock.release();
    yield();
}

pid_t Process::wait(pid_t pid, int* status) {
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
    if (status) *status = zombie->m_exitCode;

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
    m_trapFrame->a0 = 0;

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
