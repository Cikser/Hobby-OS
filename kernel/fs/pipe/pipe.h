#ifndef RISC_V_PIPE_H
#define RISC_V_PIPE_H

#include "../vfs_inode.h"
#include "../../types.h"
#include "../../proc/sync/lock.h"
#include "../../mm/kalloc/kmem_cache.h"

class PipeInode : public VfsInode {
public:
    static constexpr uint32_t PIPE_BUF_SIZE = 4096;

    PipeInode();
    ~PipeInode() override;

    int read(uint64_t offset, void* buf, uint64_t len) override;
    int write(uint64_t offset, const void* buf, uint64_t len) override;
    int readdir(uint32_t index, DirEntry* dir) override { return -1; }
    bool isDir() override { return false; }
    uint64_t size() override { return m_count; }
    int stat(InodeStat* out) override;
    uint32_t inodeNum() const override { return 0; }
    int truncate(uint64_t size) override { return -1; }
    void onOpen(uint32_t flags) override;
    void onClose(uint32_t flags) override;

    void* operator new(size_t size) {
        if (!s_cache) {
            s_cache = new KMemCache<PipeInode>();
        }
        return s_cache->alloc();
    }

    void operator delete(void* ptr) {
        s_cache->free(ptr);
    }

private:
    inline static KMemCache<PipeInode>* s_cache = nullptr;

    uint8_t* m_buf;
    uint32_t m_head;
    uint32_t m_tail;
    uint32_t m_count;
    uint32_t m_readers;
    uint32_t m_writers;
    Lock m_lock;
};

#endif