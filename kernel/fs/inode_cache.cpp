#include "inode_cache.h"
#include "../io/console/console.h"

KMemCache<CachedInode>* CachedInode::s_cache = nullptr;
LRUCache<uint32_t, CachedInode*>* InodeCache::s_cache = nullptr;
Lock InodeCache::s_lock;

static void freeCachedInode(const uint32_t& key, CachedInode*& ci) {
    if (ci) {
        VfsMount* m = ci->mount;
        VfsInode* in = ci->inode;
        delete ci;
        if (m && in) {
            m->putInode(in);
        }
    }
}

uint32_t InodeCache::makeKey(VfsMount* mount, uint32_t inodeNum) {
    return inodeNum;
}

VfsInode* InodeCache::lookup(VfsMount* mount, uint32_t inodeNum) {
    s_lock.acquire();

    if (!s_cache) {
        s_lock.release();
        return nullptr;
    }

    uint32_t key = makeKey(mount, inodeNum);
    if (!s_cache->contains(key)) {
        s_lock.release();
        return nullptr;
    }

    CachedInode* ci = s_cache->at(key);
    ci->refCount++;
    s_lock.release();
    return ci->inode;
}

VfsInode* InodeCache::insert(VfsMount* mount, uint32_t inodeNum, VfsInode* inode) {
    s_lock.acquire();

    if (!s_cache) {
        s_cache = new LRUCache<uint32_t, CachedInode*>(freeCachedInode);
    }

    uint32_t key = makeKey(mount, inodeNum);

    if (s_cache->contains(key)) {
        CachedInode* existing = s_cache->at(key);
        existing->refCount++;
        mount->putInode(inode);
        s_lock.release();
        return existing->inode;
    }

    auto ci = new CachedInode();
    ci->inode = inode;
    ci->mount = mount;
    ci->inodeNum = inodeNum;
    ci->refCount = 1;
    s_cache->insert(key, ci);

    s_lock.release();
    return inode;
}

void InodeCache::acquire(VfsMount* mount, uint32_t inodeNum) {
    s_lock.acquire();

    if (!s_cache) {
        s_lock.release();
        Console::panic("InodeCache::acquire(): cache not initialized");
    }

    uint32_t key = makeKey(mount, inodeNum);
    if (!s_cache->contains(key)) {
        s_lock.release();
        Console::panic("InodeCache::acquire(): inode not in cache");
    }

    s_cache->at(key)->refCount++;
    s_lock.release();
}

void InodeCache::release(VfsMount* mount, uint32_t inodeNum) {
    s_lock.acquire();

    if (!s_cache) {
        s_lock.release();
        return;
    }

    uint32_t key = makeKey(mount, inodeNum);
    if (!s_cache->contains(key)) {
        s_lock.release();
        return;
    }

    CachedInode* ci = s_cache->at(key);
    if (ci->refCount == 0) {
        s_lock.release();
        Console::panic("InodeCache::release(): refCount already zero");
    }

    ci->refCount--;
    if (ci->refCount == 0) {
        VfsMount* m  = ci->mount;
        VfsInode* in = ci->inode;
        s_cache->erase(key);
        delete ci;
        s_lock.release();
        m->putInode(in);
        return;
    }

    s_lock.release();
}

void InodeCache::invalidate(VfsMount* mount, uint32_t inodeNum) {
    s_lock.acquire();

    if (!s_cache) {
        s_lock.release();
        return;
    }

    uint32_t key = makeKey(mount, inodeNum);
    if (!s_cache->contains(key)) {
        s_lock.release();
        return;
    }

    CachedInode* ci = s_cache->at(key);

    if (ci->refCount == 0) {
        VfsMount* m  = ci->mount;
        VfsInode* in = ci->inode;
        s_cache->erase(key);
        delete ci;
        s_lock.release();
        m->putInode(in);
    }
    else {
        s_cache->erase(key);
        uint32_t ghostKey = key | 0x80000000u;
        if (s_cache->contains(ghostKey)) {
            VfsMount* m  = ci->mount;
            VfsInode* in = ci->inode;
            delete ci;
            s_lock.release();
            m->putInode(in);
            return;
        }
        s_cache->insert(ghostKey, ci);
        s_lock.release();
    }
}

void InodeCache::flush() {
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