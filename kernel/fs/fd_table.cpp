#include "fd_table.h"

KMemCache<FdTable>* FdTable::s_cache = nullptr;

FdTable::FdTable() : m_refCount(1) {
    for (int i = 0; i < MAX_FDS; i++)
        m_fds[i] = nullptr;
}

FdTable* FdTable::clone() const {
    m_lock.acquire();

    auto* copy = new FdTable();
    for (int i = 0; i < MAX_FDS; i++) {
        if (m_fds[i])
            copy->m_fds[i] = new File(*m_fds[i], true);
    }

    m_lock.release();
    return copy;
}

void FdTable::acquire() {
    m_lock.acquire();
    m_refCount++;
    m_lock.release();
}

void FdTable::release() {
    m_lock.acquire();
    uint32_t remaining = --m_refCount;
    m_lock.release();

    if (remaining == 0) {
        closeAll();
        delete this;
    }
}

int FdTable::alloc(File* file, int min) {
    m_lock.acquire();
    for (int i = min; i < MAX_FDS; i++) {
        if (!m_fds[i]) {
            m_fds[i] = file;
            m_lock.release();
            return i;
        }
    }
    m_lock.release();
    return -1;
}

File* FdTable::get(int fd) const {
    if (fd < 0 || fd >= MAX_FDS) return nullptr;

    m_lock.acquire();
    File* f = m_fds[fd];
    m_lock.release();
    return f;
}

int FdTable::close(int fd) {
    if (fd < 0 || fd >= MAX_FDS) return -1;

    m_lock.acquire();
    File* f = m_fds[fd];
    if (!f) {
        m_lock.release();
        return -1;
    }
    m_fds[fd] = nullptr;
    m_lock.release();

    f->close();
    delete f;
    return 0;
}

void FdTable::closeAll() {
    for (int i = 0; i < MAX_FDS; i++) {
        if (!m_fds[i]) continue;
        m_fds[i]->close();
        delete m_fds[i];
        m_fds[i] = nullptr;
    }
}

int FdTable::allocAt(int fd, File* file) {
    if (fd < 0 || fd >= MAX_FDS || !file) return -1;

    m_lock.acquire();
 
    if (m_fds[fd]) {
        File* old = m_fds[fd];
        m_fds[fd] = nullptr;
        m_lock.release();
        old->close();
        delete old;
        m_lock.acquire();
    }

    m_fds[fd] = file;
    m_lock.release();
    return fd;
}