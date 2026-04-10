#include "block_cache.h"
#include "../../mm/mem.h"

HashMap<uint64_t, CachedBlock*>* BlockCache::s_map = nullptr;
Lock BlockCache::s_lock;

void* BlockCache::lookup(uint64_t sectorNum) {
    s_lock.acquire();

    if (!s_map || !s_map->contains(sectorNum)) {
        s_lock.release();
        return nullptr;
    }

    CachedBlock* cb = s_map->at(sectorNum);
    s_lock.release();
    return cb->data;
}

void* BlockCache::insert(uint64_t sectorNum, const void* data) {
    s_lock.acquire();

    if (!s_map)
        s_map = new HashMap<uint64_t, CachedBlock*>(512);

    if (s_map->contains(sectorNum)) {
        CachedBlock* cb = s_map->at(sectorNum);
        memcpy(cb->data, data, Disk::SECTOR_SIZE);
        s_lock.release();
        return cb->data;
    }

    auto* cb = new CachedBlock();
    cb->sectorNum = sectorNum;
    memcpy(cb->data, data, Disk::SECTOR_SIZE);
    s_map->insert(sectorNum, cb);

    s_lock.release();
    return cb->data;
}

void BlockCache::invalidate(uint64_t sectorNum) {
    s_lock.acquire();

    if (!s_map || !s_map->contains(sectorNum)) {
        s_lock.release();
        return;
    }

    CachedBlock* cb = s_map->at(sectorNum);
    s_map->erase(sectorNum);
    delete cb;

    s_lock.release();
}


void BlockCache::flush() {
    s_lock.acquire();

    if (!s_map) {
        s_lock.release();
        return;
    }

    for (auto [sectorNum, cb] : *s_map) {
        if (cb) {
            delete cb;
        }
    }

    delete s_map;
    s_map = nullptr;

    s_lock.release();
}