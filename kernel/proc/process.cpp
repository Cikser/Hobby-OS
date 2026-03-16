#include "pcb.h"
#include "scheduler.h"
#include "../mm/mem.h"
#include "../mm/vm/vm.h"

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
    auto proc = new Process(pmt, 0);
    // todo elf load
    Scheduler::put(proc);
    return proc;
}

Process* Process::fork() {
    PMT* pmt = VM::createPMT();
    // todo pmt copy
    auto child = new Process(pmt, 0);
    memcpy(child->m_trapFrame, m_trapFrame, sizeof(TrapFrame));
    child->m_trapFrame->a0 = 0;
    Scheduler::put(child);
    return child;
}

int Process::exec(const char* elfPath) {
    // todo elf loader
    return -1;
}
