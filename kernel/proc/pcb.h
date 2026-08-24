#ifndef RISC_V_PCB_H
#define RISC_V_PCB_H

#include "../types.h"
#include "../trap/trapframe.h"
#include "../mm/vm/pmt.h"
#include "sync/sem.h"
#include "signal/signal_handler.h"

enum class ProcState {
    READY,
    RUNNING,
    BLOCKED,
    SLEEPING,
    STOPPED,
    ZOMBIE
};

struct Context {
    uint64_t ra;
    uint64_t sp;
    uint64_t s0;
    uint64_t s1;
    uint64_t s2;
    uint64_t s3;
    uint64_t s4;
    uint64_t s5;
    uint64_t s6;
    uint64_t s7;
    uint64_t s8;
    uint64_t s9;
    uint64_t s10;
    uint64_t s11;
};

class Process;

class PCB {
public:
    virtual ~PCB();

    static constexpr uint32_t KERNEL_STACK_SIZE = 4096 * 4;
    static constexpr uint32_t USER_STACK_SIZE = 4096 * 4;
    static constexpr uint64_t USER_STACK_TOP = 0x0000003FFFFFF000ULL;

    static void yield();
    static void dispatch();
    static void sleep(time_t sleepTime);

    pid_t pid() const { return m_pid; }
    ProcState state() const { return m_state; }
    void setState(ProcState state) { m_state = state; }
    PMT* pmt() const { return m_pmt; }
    static pid_t currentPid() { return s_running->pid(); }
    static PCB* running() { return s_running; }
    static Process* runningProcess() { return s_running->owner();}
    void setTidAddress(uint64_t addr) { m_tidAddress = addr; }
    uint64_t tidAddress() const { return m_tidAddress; }
    pid_t tgid() const { return m_tgid; }

    virtual Process* owner() = 0;
    virtual bool isProcess() = 0;
    virtual void exit(int exitCode = 0) = 0;
    virtual void clear() {};

protected:
    friend class Scheduler;
    friend class ProcList;
    friend class TrapHandler;
    friend class PCBGarbage;
    friend class SignalHandler;
    friend class SyscallHandler;
    friend class Semaphore;
    friend class FutexQueue;
    friend class Futex;

    static constexpr time_t DEFAULT_TIME_SLICE = 2;

    PCB(uint64_t entry, PMT* pmt, bool usermode = true);

    static void pcbEntry();
    static void switchContext(Context* current, Context* next);

    static PCB* s_running;
    static time_t s_timeSliceCounter;

    pid_t m_pid;
    ProcState m_state;
    Context m_context;
    TrapFrame* m_trapFrame;
    uint8_t* m_kstack;
    uint8_t* m_ustack;
    PMT* m_pmt;
    PCB* m_next;
    PCB* m_nextSleep;
    PCB* m_semNext;
    PCB* m_futexNext;
    Semaphore* m_waitingOn;
    uint64_t m_waitingOnFutexKey;
    time_t m_relativeSleepTime;
    time_t m_timeSlice;
    bool m_usermode;
    uint64_t m_entry;
    void* m_args;
    Semaphore m_waitSem;
    uint64_t m_tidAddress;
    pid_t m_tgid;
    uint64_t m_sigMask;
    SignalHandler* m_signalHandler;
    mutable Lock m_lock;

    static pid_t s_pid;
    static Lock s_pidLock;
};

#endif