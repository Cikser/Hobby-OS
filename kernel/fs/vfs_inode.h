#ifndef RISC_V_VFS_INODE_H
#define RISC_V_VFS_INODE_H

#include "../types.h"

struct DirEntry {
    char name[256];
    uint32_t inodeNum;
};

struct InodeStat {
    uint64_t size;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
    uint32_t nlinks;
};

class VfsInode {
public:
    virtual ~VfsInode()  = default;

    virtual int read(uint64_t offset, void* buf, uint64_t len) = 0;
    virtual int write(uint64_t offset, const void* buf, uint64_t len) = 0;
    virtual int readdir(uint32_t index, DirEntry* dir) = 0;
    virtual bool isDir() = 0;
    virtual uint64_t size() = 0;
    virtual int stat(InodeStat* out) = 0;
};

class VfsMount {
public:
    virtual ~VfsMount() = default;
    virtual VfsInode* getRoot() = 0;
    virtual VfsInode* getInode(uint32_t num) = 0;
    virtual void putInode(VfsInode* inode) = 0;
    virtual int mkdir(VfsInode* parent, const char* path) = 0;
    virtual VfsInode* create(VfsInode* parent, const char* path) = 0;
    virtual int unlink(VfsInode* parent, const char* path) = 0;
};

#endif