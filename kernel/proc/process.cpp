#include "pcb.h"
#include "scheduler.h"
#include "uart_inode.h"
#include "../io/console/console.h"
#include "../mm/mem.h"
#include "../mm/vm/vm.h"
#include "elf/elf.h"

KMemCache<Process>* Process::s_cache = nullptr;

Process::Process(PMT* pmt, uint64_t entry, const Process* parent) :
    PCB(entry, pmt),
    m_threads(nullptr),
    m_parent(parent)
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
    VM::destroyPMT(m_pmt);
}

Process* Process::createInit() {
    PMT* pmt = VM::createPMT();
    auto proc = new Process(pmt, -1, nullptr);

    uint64_t entry = ElfLoader::load("/bin/init", pmt);
    if (!entry) {
        Console::panic("Process:createInit(): failed to load ELF");
    }
    proc->m_entry = entry;
    return proc;
}

Process* Process::fork() {
    PMT* pmt = VM::createPMT();
    VM::copyPMT(pmt, m_pmt);
    auto child = new Process(pmt, -1, this);

    MemoryAllocator::kfreePages(child->m_ustack, USER_STACK_SIZE / MemoryLayout::PAGE_SIZE);
    child->m_ustack = (uint8_t*)MemoryLayout::p2v(child->m_pmt->translate(USER_STACK_TOP - USER_STACK_SIZE));

    memcpy(child->m_trapFrame, m_trapFrame, sizeof(TrapFrame));
    child->m_entry = m_trapFrame->sepc;
    child->m_trapFrame->a0 = 0;
    child->m_trapFrame->kstack = (uint64_t)child->m_kstack + KERNEL_STACK_SIZE;
    return child;
}

File* Process::getFile(int fd) {
    if (fd < 0 || fd >= MAX_FDS || !m_fds[fd]) return nullptr;
    return m_fds[fd];
}

int Process::exec(const char* elfPath) {
    VM::clearUserPages(m_pmt);
    uint64_t entry = ElfLoader::load(elfPath, m_pmt);
    if (!entry) return -1;
    m_entry = entry;
    m_trapFrame->sepc = entry;
    m_trapFrame->sp = USER_STACK_TOP;
    return 0;
}
