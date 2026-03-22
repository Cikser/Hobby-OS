#ifndef RISC_V_KMEM_CACHE_H
#define RISC_V_KMEM_CACHE_H

#include "../../types.h"
#include "slab.h"
#include "../../proc/sync/lock.h"

class MemCache {
public:

    void* alloc();
    int free(void *obj);

protected:
    friend class Slab;
    friend class MemoryAllocator;
    explicit MemCache(uint64_t objSize);

    void shrink();

    Slab *m_full, *m_empty, *m_partial;
    size_t m_slabSize;
    uint64_t m_objSize;
    Lock m_lock;

    void setObjSize(const uint64_t objSize);

    static MemCache s_metaCache;
    static MemCache s_memCache;
};

class KBufCache : public MemCache {

private:
    friend class MemoryAllocator;
    explicit KBufCache(const uint64_t bufSize) : MemCache(bufSize) {}
    KBufCache() : MemCache(0) {}

};

template<typename T>
class KMemCache : public MemCache {

public:
    KMemCache() : MemCache(sizeof(T)) {}

    void* operator new(size_t size) { return s_memCache.alloc(); }
    void operator delete(void *obj) { s_memCache.free(obj); }
};

#endif