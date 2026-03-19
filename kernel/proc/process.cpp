#include "pcb.h"
#include "scheduler.h"
#include "../io/console/console.h"
#include "../mm/mem.h"
#include "../mm/vm/vm.h"
#include "elf/elf.h"

KMemCache<Process>* Process::s_cache = nullptr;

Process::Process(PMT* pmt, uint64_t entry) :
    PCB(entry, pmt),
    m_threads(nullptr)
{
    uint64_t ustackPa = MemoryLayout::v2p((uint64_t)m_ustack);
    m_pmt->mapPages(
        USER_STACK_TOP - USER_STACK_SIZE,
        ustackPa,
        USER_STACK_SIZE / MemoryLayout::PAGE_SIZE,
        PMT::PAGE_USER
    );
}

Process::~Process() {
    Thread* t = m_threads;
    while (t) {
        Thread* next = t->m_nextThread;
        delete t;
        t = next;
    }
    VM::destroyPMT(m_pmt);
}

Process* Process::createInit() {
    PMT* pmt = VM::createPMT();
    auto proc = new Process(pmt, -1);

    uint64_t entry = ElfLoader::load("/bin/init", pmt);
    if (!entry) {
        Console::panic("Process:createInit(): failed to load ELF");
    }
    proc->m_entry = entry;
    return proc;
}

Process* Process::fork() const {
    PMT* pmt = VM::createPMT();
    VM::copyPMT(pmt, m_pmt);
    auto child = new Process(pmt, -1);
    memcpy(child->m_trapFrame, m_trapFrame, sizeof(TrapFrame));
    child->m_trapFrame->a0 = 0;
    child->m_trapFrame->kstack = (uint64_t)child->m_kstack + KERNEL_STACK_SIZE;
    return child;
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
