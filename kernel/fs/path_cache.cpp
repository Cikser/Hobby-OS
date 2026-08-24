#include "path_cache.h"
#include "../mm/mem.h"
#include "../mm/kalloc/kalloc.h"

KMemCache<PathEntry>* PathEntry::s_cache = nullptr;
LRUCache<const char*, PathEntry*>* PathCache::s_cache = nullptr;
Lock PathCache::s_lock;

static void freePathEntry(const char* const& key, PathEntry*& entry) {
    if (entry) {
        if (entry->path) {
            MemoryAllocator::kfree(entry->path);
        }
        delete entry;
    }
}

uint32_t PathCache::lookup(const char* path) {
    s_lock.acquire();
    if (!s_cache || !s_cache->contains(path)) {
        s_lock.release();
        return 0;
    }

    PathEntry* entry = s_cache->at(path);
    uint32_t num = entry->inodeNum;
    s_lock.release();
    return num;
}

void PathCache::insert(const char* path, uint32_t inodeNum) {
    if (inodeNum == 0) return;

    s_lock.acquire();
    if (!s_cache) {
        s_cache = new LRUCache<const char*, PathEntry*>(freePathEntry);
    }

    if (s_cache->contains(path)) {
        s_cache->at(path)->inodeNum = inodeNum;
        s_lock.release();
        return;
    }

    PathEntry* entry = new PathEntry();
    uint32_t len = strlen(path) + 1;
    entry->path = (char*)MemoryAllocator::kmalloc(len);
    memcpy(entry->path, path, len);
    entry->inodeNum = inodeNum;

    s_cache->insert(entry->path, entry);
    s_lock.release();
}

void PathCache::invalidate(const char* path) {
    s_lock.acquire();
    if (!s_cache || !s_cache->contains(path)) {
        s_lock.release();
        return;
    }

    PathEntry* entry = s_cache->at(path);
    s_cache->erase(path);

    MemoryAllocator::kfree(entry->path);
    delete entry;

    s_lock.release();
}

void PathCache::invalidatePrefix(const char* prefix) {
    s_lock.acquire();
    if (!s_cache) {
        s_lock.release();
        return;
    }

    uint32_t prefixLen = strlen(prefix);

    s_cache->eraseIf([prefix, prefixLen](const char* key, PathEntry* entry) -> bool {
        if (!entry || !entry->path) return false;

        if (strncmp(entry->path, prefix, prefixLen) == 0) {
            if (entry->path[prefixLen] == '\0' || entry->path[prefixLen] == '/') {
                return true;
            }
        }
        return false;
    });

    s_lock.release();
}

void PathCache::flush() {
    s_lock.acquire();

    if (!s_cache) {
        s_lock.release();
        return;
    }

    s_cache->flush();

    delete s_cache;
    s_cache = nullptr;

    s_lock.release();
}