#ifndef RISC_V_MEMLAYOUT_H
#define RISC_V_MEMLAYOUT_H

#include "../types.h"

class MemoryLayout {

public:
    static constexpr uint64_t PHYS_BASE = 0x80000000;
    static constexpr uint64_t PHYS_END = 0x88000000;
    static constexpr uint64_t KERNEL_BASE = 0xFFFFFFC080000000ULL;
    static constexpr uint64_t KERNEL_OFFSET = KERNEL_BASE - PHYS_BASE;

    static const uint64_t TEXT_START;
    static const uint64_t TEXT_SIZE;
    static const uint64_t HEAP_START;
    static const uint64_t HEAP_SIZE;
    static const uint64_t KERNEL_END;

    static constexpr uint64_t MMIO_BASE = 0xFFFFFFD000000000ULL;
    static constexpr uint64_t UART_BASE = MMIO_BASE + 0x10000000ULL;
    static constexpr uint64_t VIRTIO_BASE = MMIO_BASE + 0x10001000ULL;

    static constexpr uint64_t CLINT_BASE = MMIO_BASE + 0x2000000ULL;
    static constexpr uint64_t CLINT_MTIME = MMIO_BASE + 0x200BFF8ULL;
    static constexpr uint64_t CLINT_MTIMECMP = MMIO_BASE + 0x2004000ULL;
    static constexpr uint64_t TIMER_INTERVAL = 1000000;

    static constexpr uint64_t PAGE_SIZE = 0x1000;
    static constexpr uint32_t PAGE_SHIFT = 12;
    static constexpr uint32_t MEM_LIMIT_SHIFT = 27;

    static uint64_t pageRoundDown(uint64_t addr) {
        return addr & ~(PAGE_SIZE - 1);
    }

    static uint64_t pageRoundUp(uint64_t addr) {
        return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    }

    static uint64_t p2v(uint64_t phys) {
        return phys + KERNEL_OFFSET;
    }
    static uint64_t v2p(uint64_t virt) {
        return virt - KERNEL_OFFSET;
    }

};

#endif