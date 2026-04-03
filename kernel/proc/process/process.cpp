#include "process.h"

#include "garbage.h"
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

Process::Process(PMT* pmt, uint64_t entry, Process* parent) :
    PCB(entry, pmt),
    m_threads(nullptr),
    m_parent(parent),
    m_nextSibling(nullptr),
    m_firstChild(nullptr),
    m_exitCode(0),
    m_selfSem(Semaphore(0)),
    m_spaceLock(Lock()),
    m_mmap(nullptr)
{
    if (parent) {
        for (int i = 0; i < MAX_FDS; i++) {
            if (parent->m_fds[i])
                m_fds[i] = new File(*parent->m_fds[i], true);
        }
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
        m_cwdPath[0] = '/';
        m_cwdPath[1] = '\0';
        m_fds[0] = new File(UartInode::instance(), nullptr, File::O_RDONLY);
        m_fds[1] = new File(UartInode::instance(), nullptr, File::O_WRONLY);
        m_fds[2] = new File(UartInode::instance(), nullptr, File::O_WRONLY);
        m_mmap = new Mmap(m_pmt, m_segTable);
    }
}

void Process::clear() {
    Thread* t = m_threads;
    while (t) {
        Thread* next = t->m_nextThread;
        delete t;
        t = next;
    }
    for (auto& fd : m_fds) {
        if (!fd) continue;
        fd->close();
        delete fd;
    }
    delete m_mmap;
    delete m_segTable;
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
    if (!pmt) return nullptr;

    VM::copyPMT(pmt, m_pmt);
    auto child = new Process(pmt, -1, this);

    MemoryAllocator::kfreePages(child->m_ustack, USER_STACK_SIZE / MemoryLayout::PAGE_SIZE);
    child->m_ustack = (uint8_t*)MemoryLayout::p2v(child->m_pmt->translate(USER_STACK_TOP - USER_STACK_SIZE));

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
    if (!m_firstChild) {
        m_firstChild = child;
    }
    else {
        child->m_nextSibling = m_firstChild;
        m_firstChild = child;
    }
    m_lock.release();
    return child;
}

File* Process::getFile(int fd) const {
    if (fd < 0 || fd >= MAX_FDS) return nullptr;

    m_lock.acquire();
    File* f = m_fds[fd];
    m_lock.release();

    return f;
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

uint64_t Process::openFile(const char* path, uint64_t flags) {
    File* file = VFS::open(path, flags);
    if (!file) return -1;

    m_lock.acquire();
    for (int i = 0; i < MAX_FDS; i++) {
        if (!m_fds[i]) {
            m_fds[i] = file;
            m_lock.release();
            return i;
        }
    }
    m_lock.release();

    file->close();
    delete file;
    return -1;
}

int Process::closeFile(int fd) {
    if (fd < 0 || fd >= MAX_FDS) return -1;

    m_lock.acquire();
    File* f = m_fds[fd];
    if (!f) {
        m_lock.release();
        return -1;
    }
    m_fds[fd] = nullptr;
    m_lock.release();

    f->close();
    delete f;
    return 0;
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

void Process::resolveRelative(const char* path, char* out) const {
    char temp[256];
    uint32_t len = 0;

    if (path[0] == '/') {
        temp[0] = '/';
        temp[1] = '\0';
        len = 1;
    }
    else {
        uint32_t cwdLen = strlen(m_cwdPath);
        memcpy(temp, m_cwdPath, cwdLen + 1);
        len = cwdLen;
    }

    const char* p = (path[0] == '/') ? path + 1 : path;

    while (*p != '\0') {
        char component[64];
        int cLen = 0;

        while (*p != '\0' && *p != '/' && cLen < 63) {
            component[cLen++] = *p++;
        }
        component[cLen] = '\0';
        if (*p == '/') p++;
        if (cLen == 0) continue;

        if (strcmp(component, ".") == 0) {
            continue;
        }
        if (strcmp(component, "..") == 0) {
            if (len > 1) {
                len--;
                while (len > 0 && temp[len] != '/') {
                    len--;
                }
                if (len == 0) len = 1;
                temp[len] = '\0';
            }
        } else {
            if ((len > 1 && temp[len-1] != '/') || (len == 1 && temp[0] != '/'))
                temp[len++] = '/';

            if (len + cLen < 255) {
                memcpy(temp + len, component, cLen);
                len += cLen;
                temp[len] = '\0';
            }
            else {
                out[0] = '\0';
                return;
            }
        }
    }

    if (len == 0) {
        temp[0] = '/';
        temp[1] = '\0';
    }

    memcpy(out, temp, len + 1);
}

int Process::getcwd(char* buf, uint64_t size) const {
    if (!buf || size == 0) return -1;

    uint64_t needed = strlen(m_cwdPath) + 1;
    if (needed > size) return -1;

    memcpy(buf, m_cwdPath, needed);
    return 0;
}

int Process::chdir(const char* path) {
    if (!path || path[0] == '\0') return -1;

    char resolved[256];
    resolveRelative(path, resolved);
    if (resolved[0] == '\0') return -1;

    File* f = VFS::open(resolved, File::O_RDONLY);
    if (!f) return -1;

    f->close();
    delete f;

    VfsInode* inode = VFS::resolvePath(resolved);
    if (!inode) return -1;

    if (!inode->isDir()) {
        VFS::putInode(inode, inode->inodeNum());
        return -1;
    }

    uint32_t newInodeNum = inode->inodeNum();
    VFS::putInode(inode, newInodeNum);

    m_cwdInode = newInodeNum;
    strcpy(m_cwdPath, resolved);

    uint32_t len = strlen(m_cwdPath);
    if (len > 1 && m_cwdPath[len - 1] == '/')
        m_cwdPath[len - 1] = '\0';

    return 0;
}

int Process::mkdir(const char* path, uint32_t mode) const {
    if (!path || path[0] == '\0') return -1;

    char resolved[256];
    resolveRelative(path, resolved);
    if (resolved[0] == '\0') return -1;

    return VFS::mkdir(resolved);
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
