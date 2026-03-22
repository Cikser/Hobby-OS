#include "scheduler.h"

ProcList* Scheduler::m_list = nullptr;
PCB* Scheduler::m_sleepHead = nullptr;
PCB* Scheduler::m_sleepTail = nullptr;
Lock Scheduler::m_lock = Lock();

void Scheduler::put(PCB* pcb) {
    m_lock.acquire();
    if (!m_list) {
        m_list = new ProcList();
    }
    m_list->put(pcb);
    m_lock.release();
}

PCB* Scheduler::get() {
    m_lock.acquire();
    PCB* pcb = m_list ? m_list->get() : nullptr;
    m_lock.release();
    return pcb;
}

PCB* Scheduler::getSleep() {
    if (!m_sleepHead) return nullptr;

    PCB* pcb = m_sleepHead;
    m_sleepHead = m_sleepHead->m_nextSleep;
    if (!m_sleepHead) m_sleepTail = nullptr;
    pcb->m_nextSleep = nullptr;

    return pcb;
}

void Scheduler::putSleep(PCB* pcb, time_t sleepTime) {
    m_lock.acquire();

    if (!m_sleepHead) {
        m_sleepHead = m_sleepTail = pcb;
        pcb->m_relativeSleepTime = sleepTime;
        pcb->m_nextSleep = nullptr;
        m_lock.release();
        return;
    }

    PCB* cur  = m_sleepHead;
    PCB* prev = nullptr;
    time_t elapsed = 0;

    while (cur && elapsed + cur->m_relativeSleepTime <= sleepTime) {
        elapsed += cur->m_relativeSleepTime;
        prev = cur;
        cur  = cur->m_nextSleep;
    }

    pcb->m_relativeSleepTime = sleepTime - elapsed;
    pcb->m_nextSleep = cur;

    if (cur)
        cur->m_relativeSleepTime -= pcb->m_relativeSleepTime;

    if (!prev)
        m_sleepHead = pcb;
    else
        prev->m_nextSleep = pcb;

    if (!cur)
        m_sleepTail = pcb;

    m_lock.release();
}

void Scheduler::awake() {
    m_lock.acquire();

    if (!m_sleepHead) {
        m_lock.release();
        return;
    }

    m_sleepHead->m_relativeSleepTime--;

    while (m_sleepHead && m_sleepHead->m_relativeSleepTime == 0) {
        PCB* pcb = getSleep();
        pcb->setState(ProcState::READY);
        m_lock.release();
        put(pcb);
        m_lock.acquire();
    }

    m_lock.release();
}