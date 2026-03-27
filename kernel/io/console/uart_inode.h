#ifndef RISC_V_UART_INODE_H
#define RISC_V_UART_INODE_H
#include "../../fs/vfs_inode.h"

class UartInode : public VfsInode {
public:
    UartInode() = default;
    ~UartInode() override = default;

    int read(uint64_t offset, void* buf, uint64_t len) override;
    int write(uint64_t offset, const void* buf, uint64_t len) override;
    int readdir(uint32_t index, DirEntry* dir) override { return -1; }
    bool isDir() override { return false; }
    uint64_t size() override { return 0; }
    int stat(InodeStat* out) override { return -1; }
    uint32_t inodeNum() const override { return 0; }
    static VfsInode* instance() {
        if (!s_inode) {
            s_inode = new UartInode();
        }
        return s_inode;
    }

private:
    static UartInode* s_inode;
};

#endif