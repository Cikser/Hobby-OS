#include "pcb.h"
#include "../mm/vm/vm.h"

KMemCache<Thread>* Thread::s_cache = nullptr;

Thread::Thread(Process* parent, uint64_t entry, void* args) :
    PCB(entry, parent->m_pmt),
    m_parent(parent),
    m_nextThread(nullptr)
{
    uint64_t stackTop = USER_STACK_TOP -
                        m_pid * (USER_STACK_SIZE + MemoryLayout::PAGE_SIZE);

    uint64_t ustackPa = MemoryLayout::v2p((uint64_t)m_ustack);
    m_pmt->mapPages(
        stackTop - USER_STACK_SIZE,
        ustackPa,
        USER_STACK_SIZE / MemoryLayout::PAGE_SIZE,
        PMT::PAGE_USER
    );

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

Thread::~Thread() {

}
