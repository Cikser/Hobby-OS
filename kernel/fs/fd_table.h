#ifndef RISC_V_FD_TABLE_H
#define RISC_V_FD_TABLE_H

#include "../types.h"
#include "file.h"
#include "../mm/kalloc/kmem_cache.h"
#include "../proc/sync/lock.h"

class FdTable {
public:
    static constexpr int MAX_FDS = 16;

    FdTable();
    FdTable* clone() const;

    void acquire();
    void release();

    int alloc(File* file);
    int allocAt(int fd, File* file);
    File* get(int fd) const;
    int close(int fd);
    void closeAll();

    void* operator new(size_t size) {
        if (!s_cache) s_cache = new KMemCache<FdTable>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

private:
    File* m_fds[MAX_FDS];
    uint32_t m_refCount;
    mutable Lock m_lock;

    static KMemCache<FdTable>* s_cache;
};

#endif