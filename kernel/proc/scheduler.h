#ifndef RISC_V_SCHEDULER_H
#define RISC_V_SCHEDULER_H

#include "pcb.h"
#include "list/proclist.h"
#include "sync/lock.h"

class Scheduler {
public:
    static void put(PCB* pcb);
    static PCB* get();
    static bool empty() { return !m_list || m_list->empty(); }

    static void putSleep(PCB* pcb, time_t sleepTime);
    static PCB* getSleep();
    static bool emptySleep() { return m_sleepHead == nullptr; }
    static bool canAwake() { return m_sleepHead && m_sleepHead->m_relativeSleepTime == 0; }
    static void awake();

private:
    static ProcList* m_list;
    static PCB* m_sleepHead;
    static PCB* m_sleepTail;
    static Lock m_lock;
};

#endif