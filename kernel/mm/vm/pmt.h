#ifndef RISC_V_PMT_H
#define RISC_V_PMT_H

#include "../kalloc/kalloc.h"
#include "../../types.h"
#include "../../hw/memlayout.h"

class PMT {
public:
    PMT();

    void* operator new(size_t size) {
        return MemoryAllocator::kallocPage();
    }

    void operator delete(void* ptr) {
        MemoryAllocator::kfreePage(ptr);
    }

    bool mapPage(uint64_t va, uint64_t pa, uint64_t flags);
    bool mapPages(uint64_t va, uint64_t pa, uint64_t count, uint64_t flags);
    uint64_t unmapPage(uint64_t va);
    uint64_t unmapPages(uint64_t va, uint64_t count);
    uint64_t translate(uint64_t va);
    void activate() const;

    static constexpr uint64_t PAGE_V = (1ULL << 0);
    static constexpr uint64_t PAGE_R = (1ULL << 1);
    static constexpr uint64_t PAGE_W = (1ULL << 2);
    static constexpr uint64_t PAGE_X = (1ULL << 3);
    static constexpr uint64_t PAGE_U = (1ULL << 4);
    static constexpr uint64_t PAGE_G = (1ULL << 5);
    static constexpr uint64_t PAGE_A = (1ULL << 6);
    static constexpr uint64_t PAGE_D = (1ULL << 7);

    static constexpr uint64_t PAGE_KERN = PAGE_V | PAGE_R | PAGE_W | PAGE_G | PAGE_A | PAGE_D;
    static constexpr uint64_t PAGE_KERN_X = PAGE_KERN | PAGE_X;
    static constexpr uint64_t PAGE_MMIO = PAGE_V | PAGE_R | PAGE_W | PAGE_G | PAGE_A | PAGE_D;
    static constexpr uint64_t PAGE_USER = PAGE_V | PAGE_R | PAGE_W | PAGE_U | PAGE_A | PAGE_D;

private:
    friend class VM;

    static constexpr uint64_t KERNEL_PHYS = 0x80000000ULL;
    static constexpr uint64_t KERNEL_VIRT = 0xFFFFFFC080000000ULL;
    static constexpr uint64_t PHYS_MAP_VIRT = 0xFFFFFFC000000000ULL;

    static constexpr uint32_t PMT_SIZE = 512;
    static constexpr uint32_t PAGE_SIZE = MemoryLayout::PAGE_SIZE;

    static constexpr uint32_t L2_OFFSET = 30;
    static constexpr uint32_t L1_OFFSET = 21;
    static constexpr uint32_t L0_OFFSET = 12;
    static constexpr uint32_t PTE_L2_OFFSET = 28;
    static constexpr uint32_t PTE_L1_OFFSET = 19;
    static constexpr uint32_t PTE_L0_OFFSET = 10;
    static constexpr uint32_t VPN_MASK = 0x1FF;
    static constexpr uint32_t VPN_SHIFT = 12;
    static constexpr uint32_t VPN_WIDTH = 9;

    static constexpr uint64_t makePte(uint64_t phys, uint64_t flags) {
        return ((phys >> L0_OFFSET) << PTE_L0_OFFSET) | flags;
    }
    static constexpr uint64_t pte2pa(uint64_t pte) {
        return (pte >> PTE_L0_OFFSET) << L0_OFFSET;
    }
    static constexpr bool pteValid(uint64_t pte) {
        return pte & PAGE_V;
    }

    static uint64_t* pte2table(uint64_t pte) {
        return (uint64_t*)(MemoryLayout::p2v(pte2pa(pte)));
    }

    uint64_t m_entries[PMT_SIZE];

    uint64_t* walk(uint64_t va, bool alloc);

};

#endif