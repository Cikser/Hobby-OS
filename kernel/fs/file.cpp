#include "file.h"

KMemCache<File> File::s_cache = KMemCache<File>();

void* File::operator new(size_t size) {
    return s_cache.alloc();
}

void File::operator delete(void* ptr) {
    s_cache.free(ptr);
}

int File::read(void* buf, uint64_t len) const {
    if (!(m_flags & O_RDONLY)) return -1;
    if (!m_inode) return -1;

    int n = m_inode->read(m_offset, buf, len);
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
    m_mount->putInode(m_inode);
    m_inode = nullptr;
    m_mount = nullptr;
}
