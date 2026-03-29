#include "path_cache.h"
#include "../mm/mem.h"
#include "../mm/kalloc/kalloc.h"

KMemCache<PathEntry>* PathEntry::s_cache = nullptr;
HashMap<uint32_t, PathEntry*>* PathCache::s_map = nullptr;
Lock PathCache::s_lock;


uint32_t PathCache::hashPath(const char* path) {
    uint32_t h = 5381;
    while (*path)
        h = ((h << 5) + h) ^ (uint8_t)*path++;
    return h;
}

uint32_t PathCache::lookup(const char* path) {
    s_lock.acquire();

    if (!s_map) {
        s_lock.release();
        return 0;
    }

    uint32_t hash = hashPath(path);
    if (!s_map->contains(hash)) {
        s_lock.release();
        return 0;
    }

    PathEntry* entry = s_map->at(hash);

    if (strcmp(entry->path, path) != 0) {
        s_lock.release();
        return 0;
    }

    uint32_t num = entry->inodeNum;
    s_lock.release();
    return num;
}

void PathCache::insert(const char* path, uint32_t inodeNum) {
    if (inodeNum == 0) return;

    s_lock.acquire();

    if (!s_map)
        s_map = new HashMap<uint32_t, PathEntry*>();

    uint32_t hash = hashPath(path);

    if (s_map->contains(hash)) {
        PathEntry* entry = s_map->at(hash);
        if (strcmp(entry->path, path) == 0) {
            entry->inodeNum = inodeNum;
            s_lock.release();
            return;
        }
        MemoryAllocator::kfree(entry->path);
        delete entry;
        s_map->erase(hash);
    }

    PathEntry* entry = new PathEntry();
    uint32_t len = strlen(path) + 1;
    entry->path = (char*)MemoryAllocator::kmalloc(len);
    memcpy(entry->path, path, len);
    entry->inodeNum = inodeNum;

    s_map->insert(hash, entry);
    s_lock.release();
}

void PathCache::invalidate(const char* path) {
    s_lock.acquire();

    if (!s_map) {
        s_lock.release();
        return;
    }

    uint32_t hash = hashPath(path);
    if (!s_map->contains(hash)) {
        s_lock.release();
        return;
    }

    PathEntry* entry = s_map->at(hash);
    if (strcmp(entry->path, path) != 0) {
        s_lock.release();
        return;
    }

    MemoryAllocator::kfree(entry->path);
    delete entry;
    s_map->erase(hash);

    s_lock.release();
}

void PathCache::invalidatePrefix(const char* prefix) {
    // todo hash map iteration
}