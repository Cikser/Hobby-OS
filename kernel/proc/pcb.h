#ifndef RISC_V_PCB_H
#define RISC_V_PCB_H

#include "../types.h"
#include "../trap/trapframe.h"
#include "../mm/vm/pmt.h"

typedef uint64_t pid_t;

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

    void exit();
    static void dispatch();

    pid_t pid() const { return m_pid; }
    ProcState state() const { return m_state; }
    PMT* pmt() const { return m_pmt; }

//protected:
    friend class Scheduler;

    PCB(uint64_t entry, PMT* pmt, bool usermode = true);

    static void pcbEntry();
    static void switchContext(Context* current, Context* next);

    static PCB* s_running;

    pid_t m_pid;
    ProcState m_state;
    Context m_context;
    TrapFrame* m_trapFrame;
    uint8_t* m_kstack;
    uint8_t* m_ustack;
    PMT* m_pmt;
    PCB* m_next;
    bool m_usermode;
    uint64_t m_entry;

    static pid_t s_pid;
};

class Thread;

class Process : public PCB {
public:
    ~Process() override;

    static Process* createInit();
    Process* fork();
    int exec(const char* elfPath);
    Thread* createThread(void(*entry)());

private:
    friend class Thread;

    Process(PMT* pmt, uint64_t entry);
    Thread* m_threads;
};

class Thread : public PCB {
public:
    ~Thread() override;
    Thread(Process* parent, uint64_t entry);
    explicit Thread(void (*entry)());

private:
    friend class Process;

    Process* m_parent;
    Thread* m_nextThread;
};

#endif