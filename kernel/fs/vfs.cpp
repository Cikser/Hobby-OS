#include "vfs.h"
#include "inode_cache.h"
#include "path_cache.h"
#include "../io/console/console.h"
#include "../mm/mem.h"
#include "ext2/ext2.h"
#include "../proc/process/process.h"
#include "path_utils.h"

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

        uint32_t num = created->inodeNum();
        VfsInode* cached = InodeCache::insert(m_mount, num, created);

        PathCache::insert(path, num);

        return new File(cached, m_mount, flags);
    }
    uint32_t accessMode = flags & 0x3;
    bool canWrite = (accessMode == File::O_WRONLY || accessMode == File::O_RDWR);

    if ((flags & File::O_TRUNC) && canWrite) {
        if (inode->isDir()) {
            putInode(inode, inode->inodeNum());
            return nullptr;
        }
        inode->truncate(0);
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
    PathCache::invalidate(path);
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
    PathCache::insert(path, num);
    InodeCache::insert(m_mount, num, created);
    InodeCache::release(m_mount, num);
    return 0;
}

int VFS::unlink(const char* path, uint32_t flags) {
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
        bool isDir = target->isDir();
        putInode(target, targetNum);
        if (isDir && !(flags & AT_REMOVEDIR)) {
            putInode(parent, parent->inodeNum());
            return -1;
        }
    }

    int ret = m_mount->unlink(parent, name);
    putInode(parent, parent->inodeNum());

    if (ret == 0 && targetNum != 0) {
        PathCache::invalidate(path);
        InodeCache::invalidate(m_mount, targetNum);
    }

    return ret;
}

VfsInode* VFS::resolvePath(const char* path) {
    if (!m_mount) return nullptr;

    char* absolutePath = nullptr;

    if (path[0] == '/') {
        absolutePath = kstrdup(path, PATH_MAX);
    }
    else {
        Process* running = PCB::runningProcess();
        if (!running) return nullptr;
        absolutePath = running->resolveRelative(path);
    }

    if (!absolutePath) return nullptr;

    uint32_t cachedNum = PathCache::lookup(absolutePath);
    if (cachedNum != 0) {
        VfsInode* inode = getInode(cachedNum);
        if (inode) {
            MemoryAllocator::kfree(absolutePath);
            return inode;
        }
        PathCache::invalidate(absolutePath);
    }

    VfsInode* current = getInode(2);
    const char* p = absolutePath + 1;

    char* component = (char*)MemoryAllocator::kmalloc(PATH_MAX + 1);
    if (!component) {
        MemoryAllocator::kfree(absolutePath);
        return nullptr;
    }

    while (*p != '\0') {
        uint64_t len = 0;
        while (*p != '\0' && *p != '/' && len < PATH_MAX)
            component[len++] = *p++;
        component[len] = '\0';
        if (*p == '/') p++;
        if (len == 0) continue;

        if (!current->isDir()) {
            putInode(current, current->inodeNum());
            MemoryAllocator::kfree(component);
            MemoryAllocator::kfree(absolutePath);
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
            MemoryAllocator::kfree(component);
            MemoryAllocator::kfree(absolutePath);
            return nullptr;
        }
    }
    PathCache::insert(absolutePath, current->inodeNum());

    MemoryAllocator::kfree(component);
    MemoryAllocator::kfree(absolutePath);
    return current;
}

VfsInode* VFS::resolveParent(const char* path, const char** outName) {
    if (!m_mount) return nullptr;
    if (path[0] != '/') return nullptr;

    const char* lastSlash = path;
    for (const char* q = path; *q != '\0'; q++)
        if (*q == '/') lastSlash = q;

    *outName = lastSlash + 1;

    if (lastSlash == path)
        return getInode(2);

    uint64_t len = lastSlash - path;
    char* parentPath = (char*)MemoryAllocator::kmalloc(len + 1);
    if (!parentPath) return nullptr;

    memcpy(parentPath, path, len);
    parentPath[len] = '\0';

    VfsInode* result = resolvePath(parentPath);
    MemoryAllocator::kfree(parentPath);
    return result;
}