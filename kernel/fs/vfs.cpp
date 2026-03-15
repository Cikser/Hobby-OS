#include "vfs.h"
#include "../mm/mem.h"

VfsMount* Vfs::m_mount = nullptr;

void Vfs::mount(VfsMount* mount) {
    m_mount = mount;
}

File* Vfs::open(const char* path, uint32_t flags) {
    if (!m_mount) return nullptr;

    VfsInode* inode = resolvePath(path);

    if (!inode) {
        if (!(flags & File::O_CREAT)) return nullptr;

        const char* name = nullptr;
        VfsInode* parent = resolveParent(path, &name);
        if (!parent) return nullptr;

        inode = m_mount->create(parent, name);
        m_mount->putInode(parent);
        if (!inode) return nullptr;
    }

    return new File(inode, m_mount, flags);
}

int Vfs::mkdir(const char* path) {
    if (!m_mount) return -1;

    const char* name = nullptr;
    VfsInode* parent = resolveParent(path, &name);
    if (!parent) return -1;
    if (!parent->isDir()) {
        m_mount->putInode(parent);
        return -1;
    }

    if (!name || *name == '\0') {
        m_mount->putInode(parent);
        return -1;
    }

    int ret = m_mount->mkdir(parent, name);
    m_mount->putInode(parent);
    return ret;
}

int Vfs::create(const char* path) {
    if (!m_mount) return -1;

    const char* name = nullptr;
    VfsInode* parent = resolveParent(path, &name);
    if (!parent) return -1;
    if (!parent->isDir()) {
        m_mount->putInode(parent);
        return -1;
    }
    if (!name || *name == '\0') {
        m_mount->putInode(parent);
        return -1;
    }

    VfsInode* created = m_mount->create(parent, name);
    if (!created) {
        m_mount->putInode(parent);
        return -1;
    }

    m_mount->putInode(parent);
    m_mount->putInode(created);
    return 0;
}

int Vfs::unlink(const char* path) {
    if (!m_mount) return -1;

    const char* name = nullptr;
    VfsInode* parent = resolveParent(path, &name);
    if (!parent) return -1;
    if (!parent->isDir()) {
        m_mount->putInode(parent);
        return -1;
    }
    if (!name || *name == '\0') {
        m_mount->putInode(parent);
        return -1;
    }

    int ret = m_mount->unlink(parent, name);
    m_mount->putInode(parent);
    return ret;
}

VfsInode* Vfs::resolvePath(const char* path) {
    if (!m_mount) return nullptr;
    if (path[0] != '/') return nullptr;

    VfsInode* current = m_mount->getRoot();
    path++;

    while (*path != '\0') {
        char component[256];
        int len = 0;
        while (*path != '\0' && *path != '/' && len < 255) {
            component[len++] = *path++;
        }
        component[len] = '\0';
        if (*path == '/') path++;
        if (len == 0) continue;

        if (!current->isDir()) {
            m_mount->putInode(current);
            return nullptr;
        }

        DirEntry entry;
        bool found = false;
        for (uint32_t i = 0; current->readdir(i, &entry) == 0; i++) {
            if (strcmp(entry.name, component) == 0) {
                m_mount->putInode(current);
                current = m_mount->getInode(entry.inodeNum);
                found = true;
                break;
            }
        }

        if (!found) {
            m_mount->putInode(current);
            return nullptr;
        }
    }

    return current;
}

VfsInode* Vfs::resolveParent(const char* path, const char** outName) {
    if (!m_mount) return nullptr;
    if (path[0] != '/') return nullptr;

    const char* lastSlash = path;
    for (const char* p = path; *p != '\0'; p++)
        if (*p == '/') lastSlash = p;

    *outName = lastSlash + 1;

    if (lastSlash == path) {
        return m_mount->getRoot();
    }

    char parentPath[256];
    int len = lastSlash - path;
    memcpy(parentPath, path, len);
    parentPath[len] = '\0';

    return resolvePath(parentPath);
}