#include "proclist.h"

KMemCache<ProcList>* ProcList::s_cache = nullptr;

void ProcList::put(PCB* pcb) {
    if (!pcb) return;
    pcb->m_next = nullptr;

    if (!m_tail) {
        m_head = m_tail = pcb;
    } else {
        m_tail->m_next = pcb;
        m_tail = pcb;
    }
}

PCB* ProcList::get() {
    if (!m_head) return nullptr;

    PCB* pcb = m_head;
    m_head = m_head->m_next;
    if (!m_head) m_tail = nullptr;
    pcb->m_next = nullptr;
    return pcb;
}

ProcList::~ProcList() {
    PCB* pcb = m_head;
    while (pcb) {
        m_head = pcb->m_next;
        delete pcb;
        pcb = m_head;
    }
}
