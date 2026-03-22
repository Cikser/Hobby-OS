#include "scheduler.h"

ProcList* Scheduler::m_list = nullptr;
PCB* Scheduler::m_sleepHead = nullptr;
PCB* Scheduler::m_sleepTail = nullptr;

void Scheduler::put(PCB* pcb) {
    if (!m_list) {
        m_list = new ProcList();
    }
    m_list->put(pcb);
}

PCB* Scheduler::get() {
    if (!m_list) return nullptr;
    return m_list->get();
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
    if (!m_sleepHead) {
        m_sleepHead = m_sleepTail = pcb;
        pcb->m_relativeSleepTime = sleepTime;
        pcb->m_nextSleep = nullptr;
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

    if (!prev) {
        m_sleepHead = pcb;
    } else {
        prev->m_nextSleep = pcb;
    }

    if (!cur)
        m_sleepTail = pcb;
}

void Scheduler::awake() {
    if (!m_sleepHead) return;

    PCB* head = m_sleepHead;
    head->m_relativeSleepTime--;
    while (canAwake()) {
        PCB* pcb = getSleep();
        pcb->setState(ProcState::READY);
        put(pcb);
    }
}
