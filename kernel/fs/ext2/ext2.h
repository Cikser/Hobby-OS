#ifndef RISC_V_EXT2_H
#define RISC_V_EXT2_H

#include "ext2_disk.h"
#include "../vfs_inode.h"
#include "../../io/disk/disk.h"
#include "../../mm/kalloc/kmem_cache.h"
#include "../../mm/mem.h"

class Ext2Mount;

class Ext2Inode : public VfsInode {
public:
    Ext2Inode(Ext2Mount* mount, uint32_t num, const Ext2InodeDisk& raw)
        : m_mount(mount), m_num(num), m_raw(raw) {}

    void* operator new(size_t size) {
        if (!s_cache) {
            s_cache = new KMemCache<Ext2Inode>();
        }
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

    int read(uint64_t offset, void* buf, uint64_t len) override;
    int write(uint64_t offset, const void* buf, uint64_t len) override;
    int readdir(uint32_t index, DirEntry* dir) override;
    bool isDir() override;
    uint64_t size() override;
    int stat(InodeStat* out) override;

private:
    uint32_t blockMap(uint32_t logical) const;
    uint32_t blockMapAlloc(uint32_t logical);

    static KMemCache<Ext2Inode>* s_cache;

    Ext2Mount* m_mount;
    uint32_t m_num;
    Ext2InodeDisk m_raw;
};

class Ext2Mount : public VfsMount {
public:
    Ext2Mount();

    VfsInode* getRoot() override;
    VfsInode* getInode(uint32_t num) override;
    void putInode(VfsInode* inode) override;
    int mkdir(VfsInode* parent, const char* path) override;
    VfsInode* create(VfsInode* parent, const char* path) override;
    int unlink(VfsInode* parent, const char* path) override;

private:
    friend class Ext2Inode;
    void readBlock(uint32_t block, void* buf) const;
    void writeBlock(uint32_t block, const void* buf) const;
    uint32_t blockSize() const { return m_blockSize; }
    uint32_t sectorsPerBlock() const { return m_blockSize / Disk::SECTOR_SIZE; }
    Ext2BlockGroupDesc readGroupDesc(uint32_t group) const;
    void writeGroupDesc(uint32_t group, const Ext2BlockGroupDesc& desc) const;
    Ext2InodeDisk readRawInode(uint32_t num) const;
    void writeRawInode(uint32_t num, const Ext2InodeDisk& raw) const;
    uint32_t allocBlock(uint32_t preferGroup);
    void freeBlock(uint32_t block);

    static KMemCache<Ext2Mount>* s_cache;

    Ext2SuperBlock m_sb;
    uint32_t m_blockSize;
    uint32_t m_groupCount;
};

#endif