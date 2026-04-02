#include "thread.h"

#include "garbage.h"
#include "riscv.h"
#include "../../mm/vm/vm.h"

KMemCache<Thread>* Thread::s_cache = nullptr;

Thread::Thread(Process* parent, uint64_t entry, void* args) :
    PCB(entry, parent->m_pmt),
    m_parent(parent),
    m_nextThread(nullptr)
{
    uint64_t stackTop = USER_STACK_TOP -
                        m_pid * (USER_STACK_SIZE + MemoryLayout::PAGE_SIZE);

    m_parent->m_spaceLock.acquire();

    uint64_t ustackPa = MemoryLayout::v2p((uint64_t)m_ustack);
    m_pmt->mapPages(
        stackTop - USER_STACK_SIZE,
        ustackPa,
        USER_STACK_SIZE / MemoryLayout::PAGE_SIZE,
        PMT::PAGE_USER
    );

    m_parent->m_spaceLock.release();

    m_trapFrame->sp = stackTop;
    m_args = args;
}

Thread::Thread(void (*entry)(void*), void* args) :
    PCB((uint64_t)entry, nullptr, false),
    m_parent(nullptr),
    m_nextThread(nullptr)
{
    m_args = args;
}

void Thread::clear() {
    if (m_pmt) {
        if (m_parent) m_parent->m_spaceLock.acquire();

        m_pmt->unmapPages(
            USER_STACK_TOP - m_pid * (USER_STACK_SIZE + MemoryLayout::PAGE_SIZE)
                           - USER_STACK_SIZE,
            USER_STACK_SIZE / MemoryLayout::PAGE_SIZE);

        if (m_parent) m_parent->m_spaceLock.release();
    }
    if (m_ustack)
        MemoryAllocator::kfreePages(m_ustack,USER_STACK_SIZE / MemoryLayout::PAGE_SIZE);
    PCB::clear();
}

void Thread::exit(int exitCode) {
    m_lock.acquire();
    if (m_tidAddress) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        *(int*)m_tidAddress = 0;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }
    while (m_waitSem.waiting()) {
        m_waitSem.signal();
    }
    m_state = ProcState::ZOMBIE;
    clear();
    PCBGarbage::put(this);
    m_lock.release();
    yield();
}