#include "inode_cache.h"
#include "../io/console/console.h"

KMemCache<CachedInode>* CachedInode::s_cache = nullptr;
HashMap<uint32_t, CachedInode*>* InodeCache::s_map = nullptr;
Lock InodeCache::s_lock;

uint32_t InodeCache::makeKey(VfsMount* mount, uint32_t inodeNum) {
    return inodeNum;
}

VfsInode* InodeCache::lookup(VfsMount* mount, uint32_t inodeNum) {
    s_lock.acquire();

    if (!s_map) {
        s_lock.release();
        return nullptr;
    }

    uint32_t key = makeKey(mount, inodeNum);
    if (!s_map->contains(key)) {
        s_lock.release();
        return nullptr;
    }

    CachedInode* ci = s_map->at(key);
    ci->refCount++;
    s_lock.release();
    return ci->inode;
}

VfsInode* InodeCache::insert(VfsMount* mount, uint32_t inodeNum, VfsInode* inode) {
    s_lock.acquire();

    if (!s_map)
        s_map = new HashMap<uint32_t, CachedInode*>();

    uint32_t key = makeKey(mount, inodeNum);

    if (s_map->contains(key)) {
        CachedInode* existing = s_map->at(key);
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
    s_map->insert(key, ci);

    s_lock.release();
    return inode;
}

void InodeCache::acquire(VfsMount* mount, uint32_t inodeNum) {
    s_lock.acquire();

    if (!s_map) {
        s_lock.release();
        Console::panic("InodeCache::acquire(): cache not initialized");
    }

    uint32_t key = makeKey(mount, inodeNum);
    if (!s_map->contains(key)) {
        s_lock.release();
        Console::panic("InodeCache::acquire(): inode not in cache");
    }

    s_map->at(key)->refCount++;
    s_lock.release();
}

void InodeCache::release(VfsMount* mount, uint32_t inodeNum) {
    s_lock.acquire();

    if (!s_map) {
        s_lock.release();
        return;
    }

    uint32_t key = makeKey(mount, inodeNum);
    if (!s_map->contains(key)) {
        s_lock.release();
        return;
    }

    CachedInode* ci = s_map->at(key);
    if (ci->refCount == 0) {
        s_lock.release();
        Console::panic("InodeCache::release(): refCount already zero");
    }

    ci->refCount--;
    if (ci->refCount == 0) {
        VfsMount* m  = ci->mount;
        VfsInode* in = ci->inode;
        s_map->erase(key);
        delete ci;
        s_lock.release();
        m->putInode(in);
        return;
    }

    s_lock.release();
}

void InodeCache::invalidate(VfsMount* mount, uint32_t inodeNum) {
    s_lock.acquire();

    if (!s_map) {
        s_lock.release();
        return;
    }

    uint32_t key = makeKey(mount, inodeNum);
    if (!s_map->contains(key)) {
        s_lock.release();
        return;
    }

    CachedInode* ci = s_map->at(key);

    if (ci->refCount == 0) {
        VfsMount* m  = ci->mount;
        VfsInode* in = ci->inode;
        s_map->erase(key);
        delete ci;
        s_lock.release();
        m->putInode(in);
    }
    else {
        s_map->erase(key);
        uint32_t ghostKey = key | 0x80000000u;
        if (s_map->contains(ghostKey)) {
            VfsMount* m  = ci->mount;
            VfsInode* in = ci->inode;
            delete ci;
            s_lock.release();
            m->putInode(in);
            return;
        }
        s_map->insert(ghostKey, ci);
        s_lock.release();
    }
}

void InodeCache::flush() {
    s_lock.acquire();

    if (!s_map) {
        s_lock.release();
        return;
    }

    // todo hash map iteration

    delete s_map;
    s_map = nullptr;

    s_lock.release();
}