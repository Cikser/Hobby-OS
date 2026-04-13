#include "thread.h"
#include "../garbage.h"
#include "../../hw/riscv.h"
#include "../../mm/vm/vm.h"
#include "../sync/futex.h"

KMemCache<Thread>* Thread::s_cache = nullptr;

Thread::Thread(Process* parent, uint64_t entry, void* args) :
    PCB(entry, parent->m_pmt),
    m_parent(parent),
    m_nextThread(nullptr),
    m_clearTidAddr(0)
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
    m_signalHandler = parent->m_signalHandler;
    m_signalHandler->acquire();
    m_sigMask = parent->m_sigMask;
}

Thread::Thread(void (*entry)(void*), void* args) :
    PCB((uint64_t)entry, nullptr, false),
    m_parent(nullptr),
    m_nextThread(nullptr),
    m_clearTidAddr(0)
{
    m_args = args;
}

Thread::Thread(Process* parent, uint64_t entry, uint64_t userStack,
               uint64_t tls, int* childTidPtr, int* clearTidPtr) :
    PCB(entry, parent->m_pmt),
    m_parent(parent),
    m_nextThread(nullptr),
    m_clearTidAddr(0)
{
    m_trapFrame->sepc = entry;
    m_trapFrame->sp = userStack;
    m_trapFrame->tp = tls;
    m_trapFrame->a0 = 0;
    if (userStack != 0) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        uint64_t jw_addr = *(uint64_t*)(userStack + 16);
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
        m_clearTidAddr = jw_addr;
    }
    if (childTidPtr) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        *childTidPtr = (int)m_pid;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }
    m_tgid = m_parent->m_pid;
    m_signalHandler = parent->m_signalHandler;
    m_signalHandler->acquire();
    m_sigMask = parent->m_sigMask;
}

void Thread::clear() {
    if (m_pmt && m_ustack) {
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
    if (m_clearTidAddr) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        *(int*)m_clearTidAddr = 0;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
        Futex::syscall((uint32_t*)m_clearTidAddr, FUTEX_WAKE, ~0U, 0);
    }

    if (m_tidAddress) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        *(int*)m_tidAddress = 0;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);
    }
    while (m_waitSem.waiting()) {
        m_waitSem.signal();
    }
    Thread* cur = m_parent->m_threads, *prev = nullptr;
    while (cur && cur != this) {
        prev = cur;
        cur = cur->m_nextThread;
    }
    if (cur == this) {
        if (!prev) {
            m_parent->m_threads = m_parent->m_threads->m_nextThread;
        }
        else {
            prev->m_nextThread = m_nextThread;
        }
        m_nextThread = nullptr;
    }
    m_state = ProcState::ZOMBIE;
    clear();
    PCBGarbage::put(this);
    m_lock.release();
    yield();
}