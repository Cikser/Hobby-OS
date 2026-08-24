#ifndef RISC_V_SEM_H
#define RISC_V_SEM_H

#include "lock.h"
#include "../../types.h"
#include "../list/proclist.h"

class Semaphore {
public:
    explicit Semaphore(uint32_t value = 1) : m_value(value), m_blockedHead(nullptr), m_blockedTail(nullptr) {}
    ~Semaphore() = default;

    void* operator new(size_t size) {
        if (!s_cache) {
            s_cache = new KMemCache<Semaphore>();
        }
        return s_cache->alloc();
    }

    void operator delete(void* ptr) {
        s_cache->free(ptr);
    }

    void signal();
    void wait();

    void forceRemove(PCB* pcb);

    bool value() const { return m_value; }
    bool waiting() const { return m_blockedHead != nullptr; }

    static void signalWaitAtomic(Semaphore* toSignal, Semaphore* toWait);

private:
    void block();
    void unblock();

    void signalUnlocked();
    void waitUnlocked();

    void pushBlocked(PCB* pcb);
    PCB* popBlocked();

    static KMemCache<Semaphore>* s_cache;
    static Lock s_lock;

    uint32_t m_value;
    PCB* m_blockedHead;
    PCB* m_blockedTail;
    Lock m_lock;
};

#endif