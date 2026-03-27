#include "vfs.h"
#include "inode_cache.h"

#include "console.h"
#include "../mm/mem.h"
#include "ext2/ext2.h"

VfsMount* VFS::m_mount = nullptr;

void VFS::init() {
    mount(new Ext2Mount());
    Console::kprintf("VFS initialized\n");
}

void VFS::mount(VfsMount* mount) {
    m_mount = mount;
}

VfsInode* VFS::getInode(uint32_t inodeNum) {
    VfsInode* cached = InodeCache::lookup(m_mount, inodeNum);
    if (cached) return cached;

    VfsInode* fresh = m_mount->getInode(inodeNum);
    if (!fresh) return nullptr;

    return InodeCache::insert(m_mount, inodeNum, fresh);
}

void VFS::putInode(VfsInode* inode, uint32_t inodeNum) {
    InodeCache::release(m_mount, inodeNum);
}

File* VFS::open(const char* path, uint32_t flags) {
    if (!m_mount) return nullptr;

    VfsInode* inode = resolvePath(path);

    if (!inode) {
        if (!(flags & File::O_CREAT)) return nullptr;

        const char* name = nullptr;
        VfsInode* parent = resolveParent(path, &name);
        if (!parent) return nullptr;

        VfsInode* created = m_mount->create(parent, name);
        putInode(parent, parent->inodeNum());

        if (!created) return nullptr;

        VfsInode* cached = InodeCache::insert(m_mount, created->inodeNum(), created);
        return new File(cached, m_mount, flags);
    }

    return new File(inode, m_mount, flags);
}

int VFS::mkdir(const char* path) {
    if (!m_mount) return -1;

    const char* name = nullptr;
    VfsInode* parent = resolveParent(path, &name);
    if (!parent) return -1;
    if (!parent->isDir()) {
        putInode(parent, parent->inodeNum());
        return -1;
    }
    if (!name || *name == '\0') {
        putInode(parent, parent->inodeNum());
        return -1;
    }

    int ret = m_mount->mkdir(parent, name);
    putInode(parent, parent->inodeNum());
    return ret;
}

int VFS::create(const char* path) {
    if (!m_mount) return -1;

    const char* name = nullptr;
    VfsInode* parent = resolveParent(path, &name);
    if (!parent) return -1;
    if (!parent->isDir()) {
        putInode(parent, parent->inodeNum());
        return -1;
    }
    if (!name || *name == '\0') {
        putInode(parent, parent->inodeNum());
        return -1;
    }

    VfsInode* created = m_mount->create(parent, name);
    putInode(parent, parent->inodeNum());

    if (!created) return -1;

    uint32_t num = created->inodeNum();
    InodeCache::insert(m_mount, num, created);
    InodeCache::release(m_mount, num);
    return 0;
}

int VFS::unlink(const char* path) {
    if (!m_mount) return -1;

    const char* name = nullptr;
    VfsInode* parent = resolveParent(path, &name);
    if (!parent) return -1;
    if (!parent->isDir()) {
        putInode(parent, parent->inodeNum());
        return -1;
    }
    if (!name || *name == '\0') {
        putInode(parent, parent->inodeNum());
        return -1;
    }

    VfsInode* target = resolvePath(path);
    uint32_t targetNum = 0;
    if (target) {
        targetNum = target->inodeNum();
        putInode(target, targetNum);
    }

    int ret = m_mount->unlink(parent, name);
    putInode(parent, parent->inodeNum());

    if (ret == 0 && targetNum != 0)
        InodeCache::invalidate(m_mount, targetNum);

    return ret;
}

VfsInode* VFS::resolvePath(const char* path) {
    if (!m_mount) return nullptr;
    if (path[0] != '/') return nullptr;

    VfsInode* current = getInode(2);
    path++;

    while (*path != '\0') {
        char component[256];
        int len = 0;
        while (*path != '\0' && *path != '/' && len < 255)
            component[len++] = *path++;
        component[len] = '\0';
        if (*path == '/') path++;
        if (len == 0) continue;

        if (!current->isDir()) {
            putInode(current, current->inodeNum());
            return nullptr;
        }

        DirEntry entry;
        bool found = false;
        for (uint32_t i = 0; current->readdir(i, &entry) == 0; i++) {
            if (strcmp(entry.name, component) == 0) {
                putInode(current, current->inodeNum());
                current = getInode(entry.inodeNum);
                found = true;
                break;
            }
        }

        if (!found) {
            putInode(current, current->inodeNum());
            return nullptr;
        }
    }

    return current;
}

VfsInode* VFS::resolveParent(const char* path, const char** outName) {
    if (!m_mount) return nullptr;
    if (path[0] != '/') return nullptr;

    const char* lastSlash = path;
    for (const char* p = path; *p != '\0'; p++)
        if (*p == '/') lastSlash = p;

    *outName = lastSlash + 1;

    if (lastSlash == path)
        return getInode(2);

    char parentPath[256];
    int len = lastSlash - path;
    memcpy(parentPath, path, len);
    parentPath[len] = '\0';

    return resolvePath(parentPath);
}