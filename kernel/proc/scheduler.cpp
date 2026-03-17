#include "scheduler.h"

ProcList* Scheduler::m_list = nullptr;

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
