#include "scheduler.h"

PCB* Scheduler::m_head = nullptr;
PCB* Scheduler::m_tail = nullptr;

void Scheduler::put(PCB* pcb) {
    if (!pcb) return;
    pcb->m_next = nullptr;

    if (!m_tail) {
        m_head = m_tail = pcb;
    } else {
        m_tail->m_next = pcb;
        m_tail = pcb;
    }
}

PCB* Scheduler::get() {
    if (!m_head) return nullptr;

    PCB* pcb = m_head;
    m_head = m_head->m_next;
    if (!m_head) m_tail = nullptr;
    pcb->m_next = nullptr;
    return pcb;
}
