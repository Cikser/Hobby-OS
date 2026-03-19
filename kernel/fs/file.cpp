#include "file.h"

KMemCache<File>* File::s_cache = new KMemCache<File>();

File::File(const File& other, bool copyOffset) {
    m_inode = other.m_inode;
    m_flags = other.m_flags;
    m_offset = copyOffset ? other.m_offset : 0;
    m_mount = other.m_mount;
}

int File::read(void* buf, uint64_t len) {
    if (!(m_flags & O_RDONLY)) return -1;
    if (!m_inode) return -1;

    int n = m_inode->read(m_offset, buf, len);
    if (n == -1) return -1;
    m_offset += n;
    return n;
}

int File::write(const void* buf, uint64_t len) {
    if (!(m_flags & O_WRONLY)) return -1;
    if (!m_inode) return -1;

    int n = m_inode->write(m_offset, buf, len);
    if (n > 0) m_offset += n;
    return n;
}

int File::seek(uint64_t offset) {
    if (!m_inode) return -1;
    if (offset > m_inode->size()) return -1;

    m_offset = offset;
    return 0;
}

uint64_t File::tell() const {
    return m_offset;
}

void File::close() {
    if (!m_inode) return;
    if (m_mount)
        m_mount->putInode(m_inode);
    m_inode = nullptr;
    m_mount = nullptr;
}
