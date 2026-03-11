#ifndef __HW_H__
#define __HW_H__

#include "../types.h"

class MemoryLayout {

public:
    static const uint64_t KERNEL_BASE;
    static const uint64_t STACK_START;
    static const uint64_t STACK_SIZE;
    static const uint64_t TEXT_START;
    static const uint64_t TEXT_SIZE;
    static const uint64_t HEAP_START;
    static const uint64_t HEAP_SIZE;
    static const uint64_t KERNEL_END;
    static constexpr uint64_t PAGE_SIZE = 0x1000;
    static constexpr uint32_t PAGE_SHIFT = 12;
    static constexpr uint32_t MEM_LIMIT_SHIFT = 27;

    static uint64_t pageRoundDown(uint64_t addr) {
        return addr & ~(PAGE_SIZE - 1);
    }

    static uint64_t pageRoundUp(uint64_t addr) {
        return (addr + PAGE_SIZE - 1) & (~PAGE_SIZE - 1);
    }

};

#endif