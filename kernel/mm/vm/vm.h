#ifndef RISC_V_VM_H
#define RISC_V_VM_H

#include "../../types.h"

class VM {
public:
    static void init();

private:
    static constexpr uint64_t PAGE_V = (1ULL << 0);
    static constexpr uint64_t PAGE_R = (1ULL << 1);
    static constexpr uint64_t PAGE_W = (1ULL << 2);
    static constexpr uint64_t PAGE_X = (1ULL << 3);
    static constexpr uint64_t PAGE_G = (1ULL << 5);
    static constexpr uint64_t PAGE_A = (1ULL << 6);
    static constexpr uint64_t PAGE_D = (1ULL << 7);

    static constexpr uint64_t PAGE_KERN   = PAGE_V | PAGE_R | PAGE_W | PAGE_G | PAGE_A | PAGE_D;
    static constexpr uint64_t PAGE_KERN_X = PAGE_KERN | PAGE_X;
    static constexpr uint64_t PAGE_MMIO   = PAGE_V | PAGE_R | PAGE_W | PAGE_G | PAGE_A | PAGE_D;

    static constexpr uint64_t KERNEL_PHYS = 0x80000000ULL;
    static constexpr uint64_t KERNEL_VIRT = 0xFFFFFFC080000000ULL;
    static constexpr uint64_t PHYS_MAP_VIRT = 0xFFFFFFC000000000ULL;

    alignas(4096) static uint64_t s_bootPmt[512];

    static constexpr uint64_t makePte(uint64_t pa, uint64_t flags);
    static constexpr uint64_t level2Index(uint64_t va);
    static void clearBss();
    static void setupBootPmt();
};

constexpr uint64_t VM::makePte(uint64_t pa, uint64_t flags) {
    return ((pa >> 12) << 10) | flags;
}

constexpr uint64_t VM::level2Index(uint64_t va) {
    return (va >> 30) & 0x1FF;
}

#endif