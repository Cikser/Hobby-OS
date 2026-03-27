#ifndef RISC_V_VFS_H
#define RISC_V_VFS_H

#include "vfs_inode.h"
#include "file.h"

class VFS {
public:
    static void init();
    static void mount(VfsMount* mount);
    static File* open(const char* path, uint32_t flags);
    static int mkdir(const char* path);
    static int create(const char* path);
    static int unlink(const char* path);

private:
    static VfsInode* resolvePath(const char* path);
    static VfsInode* resolveParent(const char* path, const char** outName);

    static VfsInode* getInode(uint32_t inodeNum);
    static void putInode(VfsInode* inode, uint32_t inodeNum);

    static VfsMount* m_mount;
};

#endif