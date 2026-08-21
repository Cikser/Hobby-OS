#include "pipe.h"
#include "../file.h"
#include "../../mm/kalloc/kalloc.h"
#include "../../mm/mem.h"
#include "../../proc/process/process.h"
#include "../../proc/pcb.h"

PipeInode::PipeInode()
    : m_buf((uint8_t*)MemoryAllocator::kmalloc(PIPE_BUF_SIZE)),
      m_head(0), m_tail(0), m_count(0),
      m_readers(1), m_writers(1)
{}

PipeInode::~PipeInode() {
    if (m_buf) MemoryAllocator::kfree(m_buf);
}

int PipeInode::read(uint64_t offset, void* buf, uint64_t len) {
    if (!buf || len == 0) return 0;
    auto dst = (uint8_t*)buf;

    while (true) {
        m_lock.acquire();
        if (m_count > 0) {
            uint32_t toRead = (uint32_t)((len < m_count) ? len : m_count);
            for (uint32_t i = 0; i < toRead; i++) {
                dst[i] = m_buf[m_tail];
                m_tail = (m_tail + 1) % PIPE_BUF_SIZE;
            }
            m_count -= toRead;
            m_lock.release();
            return (int)toRead;
        }
        if (m_writers == 0) {
            m_lock.release();
            return 0;
        }
        m_lock.release();
        PCB::yield();
    }
}

int PipeInode::write(uint64_t offset, const void* buf, uint64_t len) {
    if (!buf) return -1;
    if (len == 0) return 0;
    auto src = (const uint8_t*)buf;
    uint64_t total = 0;

    while (total < len) {
        m_lock.acquire();
        if (m_readers == 0) {
            m_lock.release();
            Process* proc = PCB::runningProcess();
            if (proc) proc->kill(SIGPIPE);
            return total > 0 ? (int)total : -1;
        }
        uint32_t freeSpace = PIPE_BUF_SIZE - m_count;
        if (freeSpace == 0) {
            m_lock.release();
            PCB::yield();
            continue;
        }
        uint32_t toWrite = (uint32_t)(len - total);
        if (toWrite > freeSpace) toWrite = freeSpace;
        for (uint32_t i = 0; i < toWrite; i++) {
            m_buf[m_head] = src[total + i];
            m_head = (m_head + 1) % PIPE_BUF_SIZE;
        }
        m_count += toWrite;
        total += toWrite;
        m_lock.release();
    }
    return (int)total;
}

int PipeInode::stat(InodeStat* out) {
    if (!out) return -1;
    memset(out, 0, sizeof(InodeStat));
    out->st_mode = 0010000; // S_IFIFO
    out->st_nlink = 1;
    m_lock.acquire();
    out->st_size = m_count;
    m_lock.release();
    return 0;
}

void PipeInode::onOpen(uint32_t flags) {
    m_lock.acquire();
    if ((flags & 0x3) == File::O_WRONLY) m_writers++;
    else m_readers++;
    m_lock.release();
}

void PipeInode::onClose(uint32_t flags) {
    m_lock.acquire();
    if ((flags & 0x3) == File::O_WRONLY) {
        if (m_writers > 0) m_writers--;
    } else {
        if (m_readers > 0) m_readers--;
    }
    bool shouldDelete = (m_readers == 0 && m_writers == 0);
    m_lock.release();

    if (shouldDelete) delete this;
}