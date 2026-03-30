#ifndef RISC_V_BLOCK_CACHE_H
#define RISC_V_BLOCK_CACHE_H

#include "disk.h"
#include "../../types.h"
#include "../../mm/kalloc/kmem_cache.h"
#include "../../lib/hash_map.h"

struct CachedBlock {
    uint64_t sectorNum;
    uint8_t data[Disk::SECTOR_SIZE];

    void* operator new(size_t size) {
        if (!s_cache)
            s_cache = new KMemCache<CachedBlock>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

private:
    inline static KMemCache<CachedBlock>* s_cache = nullptr;
};

class BlockCache {
public:
    static void* lookup(uint64_t sectorNum);
    static void* insert(uint64_t sectorNum, const void* data);
    static void invalidate(uint64_t sectorNum);
    static void flush();

private:
    static HashMap<uint64_t, CachedBlock*>* s_map;
    static Lock s_lock;
};

#endif
