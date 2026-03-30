#include "path_cache.h"
#include "../mm/mem.h"
#include "../mm/kalloc/kalloc.h"

KMemCache<PathEntry>* PathEntry::s_cache = nullptr;
HashMap<const char*, PathEntry*>* PathCache::s_map = nullptr;
Lock PathCache::s_lock;

uint32_t PathCache::lookup(const char* path) {
    s_lock.acquire();
    if (!s_map || !s_map->contains(path)) {
        s_lock.release();
        return 0;
    }

    PathEntry* entry = s_map->at(path);
    uint32_t num = entry->inodeNum;
    s_lock.release();
    return num;
}

void PathCache::insert(const char* path, uint32_t inodeNum) {
    if (inodeNum == 0) return;

    s_lock.acquire();
    if (!s_map) s_map = new HashMap<const char*, PathEntry*>();

    if (s_map->contains(path)) {
        s_map->at(path)->inodeNum = inodeNum;
        s_lock.release();
        return;
    }

    PathEntry* entry = new PathEntry();
    uint32_t len = strlen(path) + 1;
    entry->path = (char*)MemoryAllocator::kmalloc(len);
    memcpy(entry->path, path, len);
    entry->inodeNum = inodeNum;

    s_map->insert(entry->path, entry);
    s_lock.release();
}

void PathCache::invalidate(const char* path) {
    s_lock.acquire();
    if (!s_map || !s_map->contains(path)) {
        s_lock.release();
        return;
    }

    PathEntry* entry = s_map->at(path);
    s_map->erase(path);

    MemoryAllocator::kfree(entry->path);
    delete entry;

    s_lock.release();
}

void PathCache::invalidatePrefix(const char* prefix) {
    s_lock.acquire();
    if (!s_map) {
        s_lock.release();
        return;
    }

    uint32_t prefixLen = strlen(prefix);
    Vector<const char*> keysToRemove;

    for (auto [key, entry] : *s_map) {
        if (entry && strncmp(entry->path, prefix, prefixLen) == 0) {
            if (entry->path[prefixLen] == '\0' || entry->path[prefixLen] == '/') {
                keysToRemove.pushBack(key);
            }
        }
    }

    for (uint64_t i = 0; i < keysToRemove.size(); i++) {
        const char* key = keysToRemove[i];
        PathEntry* entry = s_map->at(key);

        s_map->erase(key);
        MemoryAllocator::kfree(entry->path);
        delete entry;
    }

    s_lock.release();
}