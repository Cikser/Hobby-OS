#include "sem.h"

#include "../scheduler.h"

void Semaphore::signal() {
    if (m_blocked->empty()) {
        m_value++;
    }
    else {
        unblock();
    }
}

void Semaphore::wait() {
    if (m_value == 0) {
        block();
    }
    else {
        m_value--;
    }
}

void Semaphore::block() const {
    PCB* running = PCB::running();
    running->setState(ProcState::BLOCKED);
    m_blocked->put(running);
    PCB::dispatch();
}

void Semaphore::unblock() const {
    PCB* proc = m_blocked->get();
    proc->setState(ProcState::READY);
    Scheduler::put(proc);
}

Semaphore::~Semaphore() {
    delete m_blocked;
}
