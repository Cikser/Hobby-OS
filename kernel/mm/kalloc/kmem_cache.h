#ifndef RISC_V_KMEM_CACHE_H
#define RISC_V_KMEM_CACHE_H

#include "../../types.h"
#include "slab.h"

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

    void setObjSize(const uint64_t objSize);

    static MemCache s_metaCache;
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
};

#endif //RISC_V_KMEM_CACHE_H