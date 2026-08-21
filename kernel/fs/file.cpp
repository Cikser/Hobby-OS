#include "file.h"
#include "inode_cache.h"

KMemCache<File>* File::s_cache = new KMemCache<File>();

File::File(const File& other, bool copyOffset)
    : m_inode(other.m_inode),
      m_mount(other.m_mount),
      m_flags(other.m_flags),
      m_offset(copyOffset ? other.m_offset : 0)
{
    if (m_inode && m_inode->inodeNum() != 0)
        InodeCache::acquire(m_mount, m_inode->inodeNum());
    else if (m_inode) {
        m_inode->onOpen(m_flags);
    }
}

int File::read(void* buf, uint64_t len) {
    uint32_t mode = m_flags & 0x3;
    if (mode != O_RDONLY && mode != O_RDWR) {
        return -1;
    }

    if (!m_inode) return -1;

    int n = m_inode->read(m_offset, buf, len);
    if (n == -1) return -1;
    m_offset += n;
    return n;
}

int File::write(const void* buf, uint64_t len) {
    uint32_t mode = m_flags & 0x3;
    if (mode != O_WRONLY && mode != O_RDWR) {
        return -1;
    }

    if (!m_inode) return -1;

    int n = m_inode->write(m_flags & O_APPEND ? (uint64_t)-1 : m_offset, buf, len);
    if (n > 0) m_offset += n;
    return n;
}

int64_t File::seek(int64_t offset, uint32_t whence) {
    if (!m_inode) return -1;

    int64_t newOffset;
    switch (whence) {
    case SEEK_SET:
        newOffset = offset;
        break;
    case SEEK_CUR:
        newOffset = (int64_t)m_offset + offset;
        break;
    case SEEK_END:
        newOffset = (int64_t)m_inode->size() + offset;
        break;
    default:
        return -1;
    }

    if (newOffset < 0) return -1;
    m_offset = (uint64_t)newOffset;
    return newOffset;
}

uint64_t File::tell() const {
    return m_offset;
}

void File::close() {
    if (!m_inode) return;

    uint32_t num = m_inode->inodeNum();
    if (num != 0) {
        InodeCache::release(m_mount, num);
    }
    else if (m_mount) {
        m_mount->putInode(m_inode);
    }
    else {
        m_inode->onClose(m_flags);
    }
    m_inode = nullptr;
    m_mount = nullptr;
}

int File::fstat(InodeStat* st) const {
    if (!m_inode) return -1;
    return m_inode->stat(st);
}
