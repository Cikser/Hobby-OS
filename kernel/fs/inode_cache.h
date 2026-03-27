#ifndef HOBBY_OS_INODE_CACHE_H
#define HOBBY_OS_INODE_CACHE_H

#include "../types.h"
#include "../lib/hash_map.h"
#include "../mm/kalloc/kmem_cache.h"
#include "../proc/sync/lock.h"
#include "vfs_inode.h"

struct CachedInode {
    VfsInode* inode;
    VfsMount* mount;
    uint32_t inodeNum;
    uint32_t refCount;

    void* operator new(size_t size) {
        if (!s_cache)
            s_cache = new KMemCache<CachedInode>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

private:
    static KMemCache<CachedInode>* s_cache;
};

class InodeCache {
public:
    static VfsInode* lookup(VfsMount* mount, uint32_t inodeNum);
    static VfsInode* insert(VfsMount* mount, uint32_t inodeNum, VfsInode* inode);
    static void acquire(VfsMount* mount, uint32_t inodeNum);
    static void release(VfsMount* mount, uint32_t inodeNum);
    static void invalidate(VfsMount* mount, uint32_t inodeNum);
    static void flush();

private:
    static HashMap<uint32_t, CachedInode*>* s_map;
    static Lock s_lock;

    static uint32_t makeKey(VfsMount* mount, uint32_t inodeNum);
};

#endif
