#ifndef RISC_V_PATH_CACHE_H
#define RISC_V_PATH_CACHE_H

#include "../types.h"
#include "../lib/hash_map.h"
#include "../mm/kalloc/kmem_cache.h"
#include "../proc/sync/lock.h"

struct PathEntry {
    char* path;
    uint32_t inodeNum;

    void* operator new(size_t size) {
        if (!s_cache) s_cache = new KMemCache<PathEntry>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

    static KMemCache<PathEntry>* s_cache;
};

class PathCache {
public:
    static uint32_t lookup(const char* path);
    static void insert(const char* path, uint32_t inodeNum);
    static void invalidate(const char* path);
    static void invalidatePrefix(const char* prefix);

private:
    static uint32_t hashPath(const char* path);

    static HashMap<uint32_t, PathEntry*>* s_map;
    static Lock s_lock;
};

#endif