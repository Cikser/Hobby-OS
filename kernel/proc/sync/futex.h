#ifndef RISC_V_FUTEX_H
#define RISC_V_FUTEX_H

#include "../../types.h"
#include "lock.h"
#include "../list/proclist.h"
#include "../../lib/hash_map.h"
#include "../../mm/kalloc/kmem_cache.h"
#include "../pcb.h"

static constexpr int FUTEX_WAIT = 0;
static constexpr int FUTEX_WAKE = 1;

static constexpr int64_t FUTEX_OK = 0;
static constexpr int64_t FUTEX_EAGAIN = -11;
static constexpr int64_t FUTEX_EINVAL = -22;
static constexpr int64_t FUTEX_ETIMEDOUT = -110;

struct FutexQueue {
    PCB* waitersHead;
    PCB* waitersTail;
    uint32_t refCount;

    void* operator new(size_t size) {
        if (!s_cache) s_cache = new KMemCache<FutexQueue>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

    bool empty() const { return waitersHead == nullptr; }

    void push(PCB* pcb) {
        pcb->m_futexNext = nullptr;
        if (!waitersTail) {
            waitersHead = waitersTail = pcb;
        } else {
            waitersTail->m_futexNext = pcb;
            waitersTail = pcb;
        }
    }

    PCB* pop() {
        if (!waitersHead) return nullptr;
        PCB* pcb = waitersHead;
        waitersHead = waitersHead->m_futexNext;
        if (!waitersHead) waitersTail = nullptr;
        pcb->m_futexNext = nullptr;
        return pcb;
    }

private:
    inline static KMemCache<FutexQueue>* s_cache = nullptr;
};

class Futex {
public:
    static int64_t syscall(uint32_t* uaddr, int op, uint32_t val,
                           time_t timeout = 0);

    static void forceRemove(uint64_t physKey, PCB* pcb);

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