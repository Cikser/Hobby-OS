#include "uart_inode.h"
#include "console.h"

UartInode* UartInode::s_inode = nullptr;

int UartInode::read(uint64_t offset, void* buf, uint64_t len) {
    auto* dst = (uint8_t*)buf;
    for (uint64_t i = 0; i < len; i++) {
        dst[i] = Console::kgetc();
    }
    return len;
}

int UartInode::write(uint64_t offset, const void* buf, uint64_t len) {
    auto* src = (uint8_t*)buf;
    for (uint64_t i = 0; i < len; i++) {
        Console::kputc(src[i]);
    }
    return len;
}
