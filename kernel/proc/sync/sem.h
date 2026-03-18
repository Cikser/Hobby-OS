#ifndef RISC_V_SEM_H
#define RISC_V_SEM_H

#include "lock.h"
#include "../../types.h"
#include "../list/proclist.h"

class Semaphore {
public:
    explicit Semaphore(uint32_t value = 1) : m_value(value), m_blocked(new ProcList) {}
    ~Semaphore();

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

    bool value() const { return m_value; }
    bool waiting() const { return !m_blocked->empty(); }

    static void signalWaitAtomic(Semaphore* toSignal, Semaphore* toWait);

private:
    void block();
    void unblock() const;

    void signalUnlocked();
    void waitUnlocked();

    static KMemCache<Semaphore>* s_cache;
    static Lock s_lock;

    uint32_t m_value;
    ProcList* m_blocked;
    Lock m_lock;
};

#endif