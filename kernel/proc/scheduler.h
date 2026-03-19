#ifndef RISC_V_SCHEDULER_H
#define RISC_V_SCHEDULER_H
#include "pcb.h"
#include "list/proclist.h"

class Scheduler {
public:
    static void put(PCB* pcb);
    static PCB* get();
    static bool empty() { return m_list->empty(); }

private:
    static ProcList* m_list;

};

#endif