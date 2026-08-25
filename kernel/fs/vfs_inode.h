#ifndef RISC_V_VFS_INODE_H
#define RISC_V_VFS_INODE_H

#include "../types.h"

struct DirEntry {
    char name[256];
    uint32_t inodeNum;
    uint8_t fileType;
};

struct InodeStat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t _pad1;
    int64_t st_size;
    int32_t st_blksize;
    int32_t _pad2;
    int64_t st_blocks;

    int64_t st_atime_sec;
    int64_t st_atime_nsec;
    int64_t st_mtime_sec;
    int64_t st_mtime_nsec;
    int64_t st_ctime_sec;
    int64_t st_ctime_nsec;

    uint32_t _unused[2];
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
    virtual uint32_t inodeNum() const = 0;
    virtual int truncate(uint64_t size) = 0;
    virtual void onOpen(uint32_t flags) {}
    virtual void onClose(uint32_t flags) {}
    virtual int ioctl(uint64_t req, void* argp) { return -1; }
    virtual bool isSymlink() { return false; }
    virtual int readlink(char* buf, uint64_t bufsize) { return -1; }
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
    virtual VfsInode* symlink(VfsInode* parent, const char* name, const char* target) = 0;
    virtual int rename(VfsInode* oldParent, const char* oldName, VfsInode* newParent, const char* newName) = 0;
};

#endif