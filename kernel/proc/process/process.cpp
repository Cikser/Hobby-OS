#include "process.h"
#include "../thread/thread.h"
#include "../../io/console/uart_inode.h"
#include "../../fs/vfs.h"
#include "../../io/console/console.h"
#include "../../mm/mem.h"
#include "../../mm/vm/vm.h"
#include "../elf/elf.h"

KMemCache<Process>* Process::s_cache = nullptr;

Process::Process(PMT* pmt, uint64_t entry, Process* parent) :
    PCB(entry, pmt),
    m_threads(nullptr),
    m_parent(parent),
    m_nextSibling(nullptr),
    m_firstChild(nullptr),
    m_exitCode(0),
    m_selfSem(Semaphore(0)),
    m_spaceLock(Lock())
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
        m_fds[0] = new File(UartInode::instance(), nullptr, File::O_RDONLY);
        m_fds[1] = new File(UartInode::instance(), nullptr, File::O_WRONLY);
        m_fds[2] = new File(UartInode::instance(), nullptr, File::O_WRONLY);
    }
}

Process::~Process() {
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
    delete m_segTable;
    VM::destroyPMT(m_pmt);
}

Process* Process::createInit() {
    PMT* pmt = VM::createPMT();
    auto proc = new Process(pmt, -1, nullptr);

    uint64_t entry = ElfLoader::load("/bin/init", pmt, proc->m_segTable);
    if (!entry)
        Console::panic("Process::createInit(): failed to load ELF");

    proc->m_entry = entry;
    proc->m_segTable->setHeap(SegmentDesc::SEG_R | SegmentDesc::SEG_W, HEAP_START, HEAP_START);
    return proc;
}

Process* Process::fork() {
    m_spaceLock.acquire();

    PMT* pmt = VM::createPMT();
    VM::copyPMT(pmt, m_pmt);
    auto child = new Process(pmt, -1, this);

    MemoryAllocator::kfreePages(child->m_ustack, USER_STACK_SIZE / MemoryLayout::PAGE_SIZE);
    child->m_ustack = (uint8_t*)MemoryLayout::p2v(child->m_pmt->translate(USER_STACK_TOP - USER_STACK_SIZE));

    memcpy(child->m_trapFrame, m_trapFrame, sizeof(TrapFrame));
    child->m_entry = m_trapFrame->sepc;
    child->m_trapFrame->a0 = 0;
    child->m_trapFrame->kstack = (uint64_t)child->m_kstack + KERNEL_STACK_SIZE;

    child->m_segTable = SegmentTable::copy(m_segTable);
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

File* Process::getFile(int fd) {
    if (fd < 0 || fd >= MAX_FDS) return nullptr;

    m_lock.acquire();
    File* f = m_fds[fd];
    m_lock.release();

    return f;
}

int Process::exec(const char* elfPath) {
    m_spaceLock.acquire();
    VM::clearUserPages(m_pmt);
    m_segTable->clear();
    uint64_t entry = ElfLoader::load(elfPath, m_pmt, m_segTable);
    if (!entry) {
        m_spaceLock.release();
        return -1;
    }

    m_entry = entry;
    m_trapFrame->sepc = entry;
    m_trapFrame->sp = USER_STACK_TOP;
    m_segTable->setHeap(SegmentDesc::SEG_R | SegmentDesc::SEG_W, HEAP_START,HEAP_START);
    m_segTable->setStack(SegmentDesc::SEG_R | SegmentDesc::SEG_W,
        USER_STACK_TOP - USER_STACK_SIZE, USER_STACK_TOP);
    m_spaceLock.release();
    return 0;
}

uint64_t Process::brk(uint64_t newHeapEnd) {
    m_spaceLock.acquire();

    uint64_t heapStart = m_segTable->heap()->start;
    uint64_t heapEnd = m_segTable->heap()->end;

    if (newHeapEnd == 0 || newHeapEnd < heapStart || newHeapEnd == heapEnd) {
        m_spaceLock.release();
        return heapEnd;
    }

    if (newHeapEnd > heapEnd) {
        uint32_t pageNum = (newHeapEnd - heapEnd) / MemoryLayout::PAGE_SIZE;
        for (uint32_t i = 0; i < pageNum; i++) {
            auto page = (uint64_t)MemoryAllocator::kallocPage();
            uint64_t pagePa = MemoryLayout::v2p(page);
            if (m_pmt->mapPage(heapEnd, pagePa, PMT::PAGE_USER))
                heapEnd += MemoryLayout::PAGE_SIZE;
            else
                Console::panic("Process::brk(): failed to map page");
        }
    }
    else {
        uint32_t pageNum = (heapEnd - newHeapEnd) / MemoryLayout::PAGE_SIZE;
        for (uint32_t i = 0; i < pageNum; i++) {
            uint64_t pagePa = m_pmt->translate(heapEnd - MemoryLayout::PAGE_SIZE);
            if (m_pmt->unmapPage(heapEnd - MemoryLayout::PAGE_SIZE)) {
                MemoryAllocator::kfreePage((void*)MemoryLayout::p2v(pagePa));
                heapEnd -= MemoryLayout::PAGE_SIZE;
            }
            else
                Console::panic("Process::brk(): failed to unmap page");
        }
    }
    m_segTable->heap()->end = heapEnd;

    m_spaceLock.release();
    return heapEnd;
}

uint64_t Process::openFile(char* path, uint64_t flags) {
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
    while (m_waitSem.waiting()) {
        m_waitSem.signal();
    }

    if (m_parent) {
        m_parent->m_selfSem.signal();
    }

    m_state = ProcState::ZOMBIE;
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

    // todo garbage collection
    //delete zombie;
    return retPid;
}