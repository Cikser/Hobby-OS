#ifndef RISC_V_GARBAGE_H
#define RISC_V_GARBAGE_H
#include "../lib/list.h"
#include "pcb.h"
#include "process/process.h"

class PCBGarbage {
public:
    static void put(PCB* pcb) {
        if (!m_list) {
            m_list = new List<PCB*>();
        }
        m_list->addLast(pcb);
    }

    static void clear() {
        if (!m_list || m_list->empty()) return;

        auto it = m_list->begin();
        while (it != m_list->end()) {
            PCB* pcb = *it;

            bool safe = false;
            if (pcb->isProcess())
                safe = isSafeToDelete((Process*)pcb);
            else
                safe = isSafeToDelete(pcb);

            if (safe) {
                auto toDelete = it;
                ++it;
                m_list->remove(toDelete);
                delete pcb;
            } else {
                ++it;
            }
        }
    }

private:
    static bool isSafeToDelete(const PCB* pcb) {
        return pcb->m_next == nullptr &&
               pcb->m_nextSleep == nullptr &&
               pcb != PCB::s_running;
    }

    static bool isSafeToDelete(Process* proc) {
        return isSafeToDelete((PCB*)proc) &&
               proc->m_nextSibling == nullptr &&
               proc->m_reaped;
    }

    inline static List<PCB*>* m_list = nullptr;
};

#endif
