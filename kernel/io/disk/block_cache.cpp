#include "block_cache.h"
#include "../../mm/mem.h"

LRUCache<uint64_t, CachedBlock*>* BlockCache::s_cache = nullptr;
Lock BlockCache::s_lock;
bool BlockCache::s_flushing = false;

static void freeCachedBlock(const uint64_t& key, CachedBlock*& block) {
    if (block->dirty) {
        Disk::write(block->sectorNum, block->data, true);
    }
    delete block;
}

void* BlockCache::lookup(uint64_t sectorNum) {
    s_lock.acquire();
    if (!s_cache) {
        s_lock.release();
        return nullptr;
    }

    CachedBlock* cb = nullptr;
    if (!s_cache->get(sectorNum, cb)) {
        s_lock.release();
        return nullptr;
    }

    s_lock.release();
    return cb->data;
}

void* BlockCache::insert(uint64_t sectorNum, const void* data) {
    if (s_flushing) {
        return nullptr;
    }
    s_lock.acquire();

    if (!s_cache) {
        s_cache = new LRUCache<uint64_t, CachedBlock*>(freeCachedBlock);
    }

    if (s_cache->contains(sectorNum)) {
        CachedBlock* cb = s_cache->at(sectorNum);
        memcpy(cb->data, data, Disk::SECTOR_SIZE);
        s_lock.release();
        return cb->data;
    }

    auto* cb = new CachedBlock();
    cb->sectorNum = sectorNum;
    memcpy(cb->data, data, Disk::SECTOR_SIZE);

    s_cache->insert(sectorNum, cb);

    s_lock.release();
    return cb->data;
}

void BlockCache::invalidate(uint64_t sectorNum) {
    s_lock.acquire();

    if (!s_cache || !s_cache->contains(sectorNum)) {
        s_lock.release();
        return;
    }

    CachedBlock* cb = s_cache->at(sectorNum);
    s_cache->erase(sectorNum);
    delete cb;

    s_lock.release();
}

void BlockCache::flush() {
    s_lock.acquire();

    if (!s_cache) {
        s_lock.release();
        return;
    }

    s_flushing = true;

    s_cache->flush();

    delete s_cache;
    s_cache = nullptr;
    s_flushing = false;

    s_lock.release();
}

void BlockCache::markDirty(uint64_t sectorNum) {
    s_cache->at(sectorNum)->dirty = true;
}