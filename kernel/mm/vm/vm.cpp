#include "vm.h"

#include "pmt.h"
#include "../../io/console/console.h"
#include "../../hw/ld.h"
#include "../../hw/riscv.h"
#include "../../hw/memlayout.h"

alignas(4096) uint64_t VM::s_bootPmt[512];

void VM::init() {
    clearBss();
    setupBootPmt();
    RiscV::loadSatp((uint64_t)&s_bootPmt);
}

void VM::clearBss() {
    auto* current = (uint64_t*)_bss_start;
    auto* end     = (uint64_t*)_bss_end;
    while (current < end) *current++ = 0;
}

void VM::setupBootPmt() {
    s_bootPmt[level2Index(PMT::KERNEL_PHYS)] =
        PMT::makePte(PMT::KERNEL_PHYS, PMT::PAGE_KERN_X);

    s_bootPmt[level2Index(PMT::KERNEL_VIRT)] =
        PMT::makePte(PMT::KERNEL_PHYS, PMT::PAGE_KERN_X);

    s_bootPmt[level2Index(PMT::PHYS_MAP_VIRT)] =
        PMT::makePte(0x00000000ULL, PMT::PAGE_KERN);

    s_bootPmt[level2Index(MemoryLayout::MMIO_BASE)] =
        PMT::makePte(0x00000000ULL, PMT::PAGE_MMIO);
}

PMT* VM::createPMT() {
    auto pmt = new PMT();
    if (!pmt) return nullptr;

    for (int i = 256; i < 512; i++) {
        pmt->m_entries[i] = s_bootPmt[i];
    }

    return pmt;
}

void VM::destroyPMT(const PMT* pmt) {
    if (!pmt) return;

    for (int i = 0; i < 256; i++) {
        uint64_t l2pte = pmt->m_entries[i];
        if (!PMT::pteValid(l2pte)) continue;

        auto* l1 = (PMT*)MemoryLayout::p2v(PMT::pte2pa(l2pte));
        for (auto l1pte : l1->m_entries) {
            if (!PMT::pteValid(l1pte)) continue;

            auto* l0 = (PMT*)MemoryLayout::p2v(PMT::pte2pa(l1pte));
            for (auto l0pte : l0->m_entries) {
                if (!PMT::pteValid(l0pte)) continue;

                auto page = (void*)MemoryLayout::p2v(PMT::pte2pa(l0pte));
                MemoryAllocator::kfreePage(page);
            }
            delete l0;
        }
        delete l1;
    }
    delete pmt;
}