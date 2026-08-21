#include "tty_inode.h"
#include "../console/console.h"

void TTYInode::putc(char c) {
    Console::kputc(c);
}

char TTYInode::getc() {
    return Console::kgetc();
}

int TTYInode::write(uint64_t offset, const void* buf, uint64_t len) {
    const char* src = (const char*)buf;
    for (uint64_t i = 0; i < len; i++) {
        putc(src[i]);
    }
    return len;
}

int TTYInode::read(uint64_t offset, void* buf, uint64_t len) {
    if (len == 0) return 0;
    char* dst = (char*)buf;

    if (m_canonical) {
        while (m_lineLen == 0) {
            while (true) {
                char c = getc();

                if (c == '\b' || c == 127) {
                    if (m_lineLen > 0) {
                        m_lineLen--;
                        if (m_echo) {
                            putc('\b');
                            putc(' ');
                            putc('\b');
                        }
                    }
                    continue;
                }

                if (c == '\r' || c == '\n') {
                    if (m_lineLen < BUFFER_SIZE) {
                        m_lineBuffer[m_lineLen++] = '\n';
                    }
                    if (m_echo) {
                        putc('\r');
                        putc('\n');
                    }
                    break;
                }

                if (m_lineLen < BUFFER_SIZE) {
                    m_lineBuffer[m_lineLen++] = c;
                    if (m_echo) {
                        putc(c);
                    }
                }
            }
        }

        size_t bytesToCopy = (len < m_lineLen) ? len : m_lineLen;
        for (size_t i = 0; i < bytesToCopy; i++) {
            dst[i] = m_lineBuffer[i];
        }

        size_t remaining = m_lineLen - bytesToCopy;
        for (size_t i = 0; i < remaining; i++) {
            m_lineBuffer[i] = m_lineBuffer[bytesToCopy + i];
        }
        m_lineLen = remaining;

        return bytesToCopy;
    } 
    else {
        for (uint64_t i = 0; i < len; i++) {
            dst[i] = getc();
            if (m_echo) putc(dst[i]);
        }
        return len;
    }
}

int TTYInode::stat(InodeStat* out) {
    if (!out) return -1;

    auto* ptr = (uint8_t*)out;
    for (size_t i = 0; i < sizeof(InodeStat); i++) {
        ptr[i] = 0;
    }

    out->st_dev = 0;
    out->st_ino = 0;
    
    constexpr uint32_t S_IFCHR = 0020000;
    out->st_mode = S_IFCHR | 0666;
    
    out->st_nlink = 1;
    out->st_uid = 0;
    out->st_gid = 0;
    
    out->st_rdev = (4 << 8) | 0; 
    
    out->st_size = 0;
    out->st_blksize = 4096;
    out->st_blocks = 0;

    return 0;
}