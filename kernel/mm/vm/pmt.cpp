#include "pmt.h"
#include "../mem.h"
#include "../../io/console/console.h"
#include "../../hw/riscv.h"

PMT::PMT() {
    memset(m_entries, 0, PAGE_SIZE);
}

uint64_t* PMT::walk(uint64_t va, bool alloc) {
    uint64_t* table = m_entries;
    for (int level = 2; level > 0; level--) {
        uint64_t pageNum = (va >> (VPN_SHIFT + level * VPN_WIDTH)) & VPN_MASK;
        uint64_t* pte = table + pageNum;

        if (pteValid(*pte)) {
            table = pte2table(*pte);
        }
        else {
            if (!alloc) return nullptr;

            auto newTable = new PMT();
            if (!newTable) return nullptr;

            uint64_t pa = MemoryLayout::v2p((uint64_t)newTable);
            *pte = makePte(pa, PAGE_V);
            table = (uint64_t*)newTable;
        }
    }
    uint64_t vpn0 = (va >> (VPN_SHIFT)) & VPN_MASK;
    return &table[vpn0];
}

bool PMT::mapPage(uint64_t va, uint64_t pa, uint64_t flags) {
    uint64_t* pte = walk(va, true);
    if (!pte) return false;

    if (pteValid(*pte))
        Console::panic("PMT::mapPage: page already mapped");

    *pte = makePte(pa, flags);
    return true;
}

bool PMT::mapPages(uint64_t va, uint64_t pa, uint64_t count, uint64_t flags) {
    for (int i = 0; i < count; i++) {
        if (!mapPage(va + i * PAGE_SIZE, pa + i * PAGE_SIZE, flags))
            return false;
    }
    return true;
}

uint64_t PMT::unmapPage(uint64_t va) {
    uint64_t* pte = walk(va, false);
    if (!pte || !pteValid(*pte)) return 0;

    uint64_t pa = pte2pa(*pte);
    *pte = 0;
    return pa;
}

uint64_t PMT::unmapPages(uint64_t va, uint64_t count) {
    uint64_t ret = unmapPage(va);
    if (!ret) return 0;
    for (int i = 1; i < count; i++) {
        if (!unmapPage(va + i * PAGE_SIZE))
            return 0;
    }
    return ret;
}

uint64_t PMT::translate(uint64_t va) {
    uint64_t* pte = walk(va, false);
    if (!pte || !pteValid(*pte)) return 0;
    return pte2pa(*pte);
}

void PMT::activate() const {
    uint64_t addr = (uint64_t)m_entries;
    uint64_t pa;

    if (addr >= MemoryLayout::PHYS_BASE && addr < MemoryLayout::PHYS_END) {
        pa = addr;
    } else {
        pa = MemoryLayout::v2p(addr);
    }

    RiscV::loadSatp(pa);
}
