#ifndef RISC_V_COND_H
#define RISC_V_COND_H
#include "sem.h"

class Condition {
public:
    explicit Condition(Semaphore* mutex) : m_mutex(mutex), m_cond(Semaphore(0)) {}

    void* operator new(size_t size){
        if (!s_cache) {
            s_cache = new KMemCache<Condition>();
        }
        return s_cache->alloc();
    }

    void operator delete(void* ptr) {
        s_cache->free(ptr);
    }

    void wait();
    void signal();

private:
    static KMemCache<Condition>* s_cache;
    Semaphore* m_mutex;
    Semaphore m_cond;

};

class CondMutex {
public:
    explicit CondMutex(Semaphore* mutex) : m_mutex(mutex) {
        m_mutex->wait();
    }

    ~CondMutex() {
        m_mutex->signal();
    }

private:
    Semaphore* m_mutex;

};

#endif