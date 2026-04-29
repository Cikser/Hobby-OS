#ifndef RISC_V_FILE_H
#define RISC_V_FILE_H

#include "../types.h"
#include "vfs_inode.h"
#include "../mm/kalloc/kmem_cache.h"

class File {
public:
    File(VfsInode* inode, VfsMount* mount, uint32_t flags)
        : m_inode(inode), m_mount(mount), m_flags(flags), m_offset(0) {}
    File(const File& other, bool copyOffset = false);

    void* operator new(size_t size) {
        if (!s_cache)
            s_cache = new KMemCache<File>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

    int read(void* buf, uint64_t len);
    int write(const void* buf, uint64_t len);
    int64_t seek(int64_t offset, uint32_t whence);
    uint64_t tell() const;
    uint32_t flags() const { return m_flags; }
    void close();
    int fstat(InodeStat* st) const;

    static constexpr uint32_t O_RDONLY = 0x0;
    static constexpr uint32_t O_WRONLY = 0x1;
    static constexpr uint32_t O_RDWR = 0x2;
    static constexpr uint32_t O_CREAT = 0x40;
    static constexpr uint32_t O_TRUNC = 0x200;
    static constexpr uint32_t O_APPEND = 0x400;

    static constexpr uint32_t SEEK_SET = 0x0;
    static constexpr uint32_t SEEK_CUR = 0x1;
    static constexpr uint32_t SEEK_END = 0x2;

private:
    static KMemCache<File>* s_cache;

    VfsInode* m_inode;
    VfsMount* m_mount;
    uint32_t m_flags;
    uint64_t m_offset;
};

#endif