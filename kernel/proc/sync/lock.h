#ifndef RISC_V_LOCK_H
#define RISC_V_LOCK_H

#include "../../types.h"
#include "../../mm/kalloc/kmem_cache.h"

class Lock {
public:
    Lock() : m_locked(0), m_pie(0) {};

    void* operator new(size_t){
        if (!s_cache)
            s_cache = new KMemCache<Lock>();
        return s_cache->alloc();
    }

    void operator delete(void* ptr) {
        s_cache->free(ptr);
    }

    void acquire();
    void release();

private:
    static KMemCache<Lock>* s_cache;
    uint32_t m_locked;
    uint32_t m_pie;

};

#endif