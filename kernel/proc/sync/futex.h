#ifndef RISC_V_FUTEX_H
#define RISC_V_FUTEX_H

#include "../../types.h"
#include "lock.h"
#include "../list/proclist.h"
#include "../../lib/hash_map.h"
#include "../../mm/kalloc/kmem_cache.h"

static constexpr int FUTEX_WAIT = 0;
static constexpr int FUTEX_WAKE = 1;

static constexpr int64_t FUTEX_OK = 0;
static constexpr int64_t FUTEX_EAGAIN = -11;
static constexpr int64_t FUTEX_EINVAL = -22;
static constexpr int64_t FUTEX_ETIMEDOUT = -110;

struct FutexQueue {
    ProcList* waiters;
    uint32_t refCount;

    void* operator new(size_t size) {
        if (!s_cache) s_cache = new KMemCache<FutexQueue>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

private:
    inline static KMemCache<FutexQueue>* s_cache = nullptr;
};

class Futex {
public:
    static int64_t syscall(uint32_t* uaddr, int op, uint32_t val,
                           time_t timeout = 0);

private:
    static uint64_t toPhysKey(uint32_t* uaddr);

    static int64_t wait(uint64_t physKey, uint32_t* uaddr, uint32_t val, time_t timeout);
    static int64_t wake(uint64_t physKey, uint32_t count);

    static FutexQueue* getQueue(uint64_t physKey);
    static void tryFreeQueue(uint64_t physKey);

    static bool isAligned(uint32_t* addr) { return ((uint64_t)addr & 0x3) == 0;}

    static Lock s_lock;
    static HashMap<uint64_t, FutexQueue*>* s_table;
};

#endif