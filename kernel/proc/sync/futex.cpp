#include "futex.h"
#include "../pcb.h"
#include "../scheduler.h"
#include "../../hw/riscv.h"
#include "../../mm/vm/pmt.h"

Lock Futex::s_lock;
HashMap<uint64_t, FutexQueue*>* Futex::s_table = nullptr;

uint64_t Futex::toPhysKey(uint32_t* uaddr) {
    if (!uaddr) return 0;

    if (!isAligned(uaddr)) return 0;

    PMT* pmt = PCB::running()->pmt();
    if (!pmt) return 0;

    uint64_t pa = pmt->translate((uint64_t)uaddr);
    return pa;
}

FutexQueue* Futex::getQueue(uint64_t physKey) {
    if (!s_table)
        s_table = new HashMap<uint64_t, FutexQueue*>();

    if (s_table->contains(physKey))
        return s_table->at(physKey);

    auto* q = new FutexQueue();
    q->waitersHead = nullptr;
    q->waitersTail = nullptr;
    q->refCount = 0;
    s_table->insert(physKey, q);
    return q;
}

void Futex::tryFreeQueue(uint64_t physKey) {
    if (!s_table || !s_table->contains(physKey)) return;

    FutexQueue* q = s_table->at(physKey);
    if (q->refCount == 0 && q->empty()) {
        s_table->erase(physKey);
        delete q;
    }
}


int64_t Futex::wait(uint64_t physKey, uint32_t* uaddr, uint32_t val, time_t timeout) {
    s_lock.acquire();

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    uint32_t current = *uaddr;
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    if (current != val) {
        s_lock.release();
        return FUTEX_EAGAIN;
    }

    FutexQueue* q = getQueue(physKey);
    q->refCount++;

    PCB* me = PCB::running();
    me->m_waitingOnFutexKey = physKey;
    me->setState(ProcState::BLOCKED);
    q->push(me);

    s_lock.release();

    if (timeout > 0) {
        me->setState(ProcState::SLEEPING);
        Scheduler::putSleep(me, timeout);
    }

    PCB::yield();

    me->m_waitingOnFutexKey = 0;

    s_lock.acquire();
    q->refCount--;
    tryFreeQueue(physKey);
    s_lock.release();

    if (timeout > 0) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
        uint32_t nowVal = *uaddr;
        RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

        if (nowVal == val) return FUTEX_ETIMEDOUT;
    }

    return FUTEX_OK;
}

int64_t Futex::wake(uint64_t physKey, uint32_t count) {
    s_lock.acquire();

    if (!s_table || !s_table->contains(physKey)) {
        s_lock.release();
        return 0;
    }

    FutexQueue* q = s_table->at(physKey);
    int64_t woken = 0;

    while (woken < (int64_t)count) {
        PCB* pcb = q->pop();
        if (!pcb) break;

        if (pcb->state() == ProcState::SLEEPING)
            Scheduler::wakeUp(pcb);
        else
            pcb->setState(ProcState::READY);
        s_lock.release();
        Scheduler::put(pcb);
        s_lock.acquire();

        woken++;
    }

    tryFreeQueue(physKey);
    s_lock.release();

    return woken;
}

int64_t Futex::syscall(uint32_t* uaddr, int op, uint32_t val, time_t timeout) {
    uint64_t physKey = toPhysKey(uaddr);
    if (!physKey) return FUTEX_EINVAL;

    switch (op) {
    case FUTEX_WAIT:
        return wait(physKey, uaddr, val, timeout);

    case FUTEX_WAKE:
        return wake(physKey, val);

    default:
        return FUTEX_EINVAL;
    }
}

void Futex::forceRemove(uint64_t physKey, PCB* pcb) {
    s_lock.acquire();
    if (s_table && s_table->contains(physKey)) {
        FutexQueue* q = s_table->at(physKey);
        if (q->waitersHead == pcb) {
            q->waitersHead = pcb->m_futexNext;
            if (!q->waitersHead) q->waitersTail = nullptr;
        } else {
            PCB* cur = q->waitersHead;
            while (cur && cur->m_futexNext != pcb) cur = cur->m_futexNext;
            if (cur) {
                cur->m_futexNext = pcb->m_futexNext;
                if (q->waitersTail == pcb) q->waitersTail = cur;
            }
        }
        pcb->m_futexNext = nullptr;
    }
    s_lock.release();
}