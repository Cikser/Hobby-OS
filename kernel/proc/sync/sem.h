#ifndef RISC_V_SEM_H
#define RISC_V_SEM_H

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

private:
    void block() const;
    void unblock() const;

    static KMemCache<Semaphore>* s_cache;

    uint32_t m_value;
    ProcList* m_blocked;
};

#endif