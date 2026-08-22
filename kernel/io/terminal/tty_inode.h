#ifndef RISC_V_TTY_INODE_H
#define RISC_V_TTY_INODE_H

#include "../../fs/vfs_inode.h"
#include "../../mm/kalloc/kmem_cache.h"
#include "termios.h"

class TTYInode : public VfsInode {
public:
    TTYInode();
    ~TTYInode() override = default;

    int read(uint64_t offset, void* buf, uint64_t len) override;
    int write(uint64_t offset, const void* buf, uint64_t len) override;
    int readdir(uint32_t index, DirEntry* dir) override { return -1; }
    bool isDir() override { return false; }
    uint64_t size() override { return 0; }
    int stat(InodeStat* out) override;
    uint32_t inodeNum() const override { return 0; }
    int truncate(uint64_t size) override { return 0; }
    int ioctl(uint64_t req, void* argp) override;

    void* operator new(size_t size) {
        if (!s_cache) {
            s_cache = new KMemCache<TTYInode>();
        }
        return s_cache->alloc();
    }

    void operator delete(void* ptr) {
        s_cache->free(ptr);
    }

private:
    char getc();
    void putc(char c);

    inline static KMemCache<TTYInode>* s_cache = nullptr;

    static constexpr size_t BUFFER_SIZE = 1024;

    char m_lineBuffer[BUFFER_SIZE];
    size_t m_lineLen = 0;
    
    bool m_echo = true;
    bool m_canonical = true;

    ktermios m_termios;
    kwinsize m_winsize;
};

#endif