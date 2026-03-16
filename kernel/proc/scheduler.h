#ifndef RISC_V_SCHEDULER_H
#define RISC_V_SCHEDULER_H
#include "pcb.h"

class Scheduler {
public:
    static void put(PCB* pcb);
    static PCB* get();

private:
    static PCB* m_head;
    static PCB* m_tail;

};

#endif