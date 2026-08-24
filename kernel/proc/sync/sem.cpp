#include "sem.h"
#include "../scheduler.h"
#include "../../io/console/console.h"

KMemCache<Semaphore>* Semaphore::s_cache = nullptr;
Lock Semaphore::s_lock = Lock();

void Semaphore::pushBlocked(PCB* pcb) {
    pcb->m_semNext = nullptr;
    if (!m_blockedTail) {
        m_blockedHead = m_blockedTail = pcb;
    } else {
        m_blockedTail->m_semNext = pcb;
        m_blockedTail = pcb;
    }
}

PCB* Semaphore::popBlocked() {
    if (!m_blockedHead) return nullptr;
    PCB* pcb = m_blockedHead;
    m_blockedHead = m_blockedHead->m_semNext;
    if (!m_blockedHead) m_blockedTail = nullptr;
    pcb->m_semNext = nullptr;
    pcb->m_waitingOn = nullptr;
    return pcb;
}

void Semaphore::signal() {
    m_lock.acquire();
    if (!m_blockedHead) {
        m_value++;
    }
    else {
        unblock();
    }
    m_lock.release();
}

void Semaphore::wait() {
    m_lock.acquire();
    if (m_value == 0) {
        block();
    }
    else {
        m_value--;
        m_lock.release();
    }
}

void Semaphore::block() {
    PCB* running = PCB::running();
    running->setState(ProcState::BLOCKED);
    running->m_waitingOn = this;
    pushBlocked(running);
    m_lock.release();
    PCB::yield();
    running->m_waitingOn = nullptr;
}

void Semaphore::unblock() {
    PCB* proc = popBlocked();
    proc->setState(ProcState::READY);
    Scheduler::put(proc);
}

void Semaphore::signalWaitAtomic(Semaphore* toSignal, Semaphore* toWait) {
    s_lock.acquire();
    toSignal->signalUnlocked();
    toWait->m_lock.acquire();
    if (toWait->m_value == 0) {
        PCB* running = PCB::running();
        running->setState(ProcState::BLOCKED);
        toWait->pushBlocked(running);
        toWait->m_lock.release();
        s_lock.release();
        PCB::yield();
    } else {
        toWait->m_value--;
        toWait->m_lock.release();
        s_lock.release();
    }
}

void Semaphore::signalUnlocked() {
    if (!m_blockedHead) {
        m_value++;
    }
    else {
        unblock();
    }
}

void Semaphore::waitUnlocked() {
    if (m_value == 0) {
        PCB* running = PCB::running();
        running->setState(ProcState::BLOCKED);
        pushBlocked(running);
        PCB::yield();
    }
    else {
        m_value--;
    }
}

void Semaphore::forceRemove(PCB* pcb) {
    if (m_blockedHead == pcb) {
        m_blockedHead = pcb->m_semNext;
        if (!m_blockedHead) m_blockedTail = nullptr;
        pcb->m_semNext = nullptr;
        return;
    }
    PCB* cur = m_blockedHead;
    while (cur && cur->m_semNext != pcb) cur = cur->m_semNext;
    if (cur) {
        cur->m_semNext = pcb->m_semNext;
        if (m_blockedTail == pcb) m_blockedTail = cur;
        pcb->m_semNext = nullptr;
    }
}