#ifndef RISC_V_PAGE_META_H
#define RISC_V_PAGE_META_H

#include "../../types.h"
#include "../kalloc/kmem_cache.h"
#include "../../proc/sync/lock.h"

struct PageMeta {
    uint64_t pa;
    uint32_t refcount;
    PageMeta* next;

    void* operator new(size_t size) {
        if (!s_cache)
            s_cache = new KMemCache<PageMeta>();
        return s_cache->alloc();
    }

    void operator delete(void* ptr) {
        s_cache->free(ptr);
    }

private:
    static KMemCache<PageMeta>* s_cache;
};

class PageRefCount {
public:
    static void init();

    static void incRef(uint64_t pa);
    static bool decRef(uint64_t pa);
    static uint32_t getRef(uint64_t pa);

private:
    static constexpr uint32_t BUCKET_COUNT = 1024;
    static constexpr uint32_t BUCKET_MASK = BUCKET_COUNT - 1;

    static PageMeta* s_buckets[BUCKET_COUNT];
    static Lock s_lock;
    static bool s_initialized;

    static uint32_t hash(uint64_t pa);
    static PageMeta* find(uint64_t pa);
    static void erase(uint64_t pa);
};

#endif