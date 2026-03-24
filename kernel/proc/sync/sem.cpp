#include "sem.h"
#include "../scheduler.h"
#include "../../io/console/console.h"

KMemCache<Semaphore>* Semaphore::s_cache = nullptr;
Lock Semaphore::s_lock = Lock();

void Semaphore::signal() {
    m_lock.acquire();
    if (m_blocked->empty()) {
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
    m_blocked->put(running);
    m_lock.release();
    PCB::yield();
}

void Semaphore::unblock() const {
    PCB* proc = m_blocked->get();
    proc->setState(ProcState::READY);
    Scheduler::put(proc);
}

Semaphore::~Semaphore() {
    delete m_blocked;
}

void Semaphore::signalWaitAtomic(Semaphore* toSignal, Semaphore* toWait) {
    s_lock.acquire();
    toSignal->signalUnlocked();
    toWait->m_lock.acquire();
    if (toWait->m_value == 0) {
        PCB* running = PCB::running();
        running->setState(ProcState::BLOCKED);
        toWait->m_blocked->put(running);
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
    if (m_blocked->empty()) {
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
        m_blocked->put(running);
        PCB::yield();
    }
    else {
        m_value--;
    }
}
