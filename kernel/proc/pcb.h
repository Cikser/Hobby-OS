#ifndef RISC_V_PCB_H
#define RISC_V_PCB_H

#include "file.h"
#include "../types.h"
#include "../trap/trapframe.h"
#include "../mm/vm/pmt.h"
#include "../mm/vm/segment.h"
#include "../sync/sem.h"

typedef uint64_t pid_t;
typedef uint64_t time_t;

enum class ProcState {
    READY,
    RUNNING,
    BLOCKED,
    SLEEPING,
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

class PCB {
public:
    static constexpr uint32_t KERNEL_STACK_SIZE = 4096 * 4;
    static constexpr uint32_t USER_STACK_SIZE = 4096 * 4;
    static constexpr uint64_t USER_STACK_TOP = 0x0000003FFFFFF000ULL;

    virtual ~PCB();

    static void dispatch();
    static void sleep(time_t sleepTime);

    pid_t pid() const { return m_pid; }
    ProcState state() const { return m_state; }
    void setState(ProcState state) { m_state = state; }
    PMT* pmt() const { return m_pmt; }
    static pid_t currentPid() { return s_running->pid(); }
    static PCB* running() { return s_running; }

    virtual PCB* fork() = 0;
    virtual File* getFile(int fd) = 0;
    virtual uint64_t brk(uint64_t newHeapEnd) = 0;
    virtual uint64_t openFile(char* path, uint64_t flags) = 0;
    virtual int closeFile(int fd) = 0;
    virtual SegmentTable* segmentTable() const = 0;
    virtual void exit(int exitCode = 0) = 0;
    virtual pid_t wait(pid_t pid, int* status = nullptr) = 0;
    virtual int exec(const char* elfPath) = 0;

protected:
    friend class Scheduler;
    friend class ProcList;
    friend class TrapHandler;

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
    time_t m_relativeSleepTime;
    time_t m_timeSlice;
    bool m_usermode;
    uint64_t m_entry;
    void* m_args;
    Semaphore m_waitSem;
    Lock m_lock;

    static pid_t s_pid;
    static Lock s_pidLock;
};

#endif