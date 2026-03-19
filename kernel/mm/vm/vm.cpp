#include "vm.h"

#include "mem.h"
#include "pmt.h"
#include "../../hw/memlayout.h"

alignas(4096) uint64_t VM::s_bootPmt[512];

extern "C" char _bss_start_pa[];
extern "C" char _bss_end_pa[];
extern "C" char _boot_pmt_pa[];

void VM::bootstrap() {
    constexpr uint64_t KERNEL_PHYS = MemoryLayout::PHYS_BASE;
    constexpr uint64_t PHYS_MAP_VIRT = MemoryLayout::KERNEL_OFFSET;

    auto* bss_cur = (uint64_t*)_bss_start_pa;
    auto* bss_end = (uint64_t*)_bss_end_pa;
    while (bss_cur < bss_end) *bss_cur++ = 0;

    auto* boot = (uint64_t*)_boot_pmt_pa;
    for (int i = 0; i < 512; i++) boot[i] = 0;

    boot[(KERNEL_PHYS >> 30) & 0x1FF] =
        ((KERNEL_PHYS >> 12) << 10) | (PMT::PAGE_KERN_X | PMT::PAGE_A);

    boot[(MemoryLayout::KERNEL_BASE >> 30) & 0x1FF] =
        ((KERNEL_PHYS >> 12) << 10) | (PMT::PAGE_KERN_X | PMT::PAGE_A);

    boot[(PHYS_MAP_VIRT >> 30) & 0x1FF] =
        ((0x00000000ULL >> 12) << 10) | (PMT::PAGE_KERN | PMT::PAGE_A | PMT::PAGE_D);

    boot[(MemoryLayout::MMIO_BASE >> 30) & 0x1FF] =
        ((0x00000000ULL >> 12) << 10) | (PMT::PAGE_MMIO | PMT::PAGE_A | PMT::PAGE_D);
}

extern "C" void vm_bootstrap() __attribute__((section(".text.init")));
extern "C" void vm_bootstrap() { VM::bootstrap(); }

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

bool VM::copyPMT(PMT* dst, PMT* src) {
    for (int i = 0; i < 256; i++) {
        if (!PMT::pteValid(src->m_entries[i])) continue;

        auto l1src = (PMT*)MemoryLayout::p2v(PMT::pte2pa(src->m_entries[i]));
        auto l1dst = new PMT();

        for (int j = 0; j < 512; j++) {
            if (!PMT::pteValid(l1src->m_entries[j])) continue;

            auto* l0src = (PMT*)MemoryLayout::p2v(PMT::pte2pa(l1src->m_entries[j]));
            auto* l0dst = new PMT();

            for (int k = 0; k < 512; k++) {
                if (!PMT::pteValid(l0src->m_entries[k])) continue;

                void* newPage = MemoryAllocator::kallocPage();
                if (!newPage) return false;

                uint64_t srcPa = PMT::pte2pa(l0src->m_entries[k]);
                memcpy(newPage, (void*)MemoryLayout::p2v(srcPa), MemoryLayout::PAGE_SIZE);

                uint64_t flags = l0src->m_entries[k] & 0x3FF;
                l0dst->m_entries[k] = PMT::makePte(MemoryLayout::v2p((uint64_t)newPage), flags);
            }

            l1dst->m_entries[j] = PMT::makePte(MemoryLayout::v2p((uint64_t)l0dst), PMT::PAGE_V);
        }

        dst->m_entries[i] = PMT::makePte(MemoryLayout::v2p((uint64_t)l1dst), PMT::PAGE_V);
    }
    return true;
}

void VM::clearUserPages(PMT* pmt) {
    for (int i = 0; i < 256; i++) {
        if (!PMT::pteValid(pmt->m_entries[i])) continue;

        auto* l1 = (PMT*)MemoryLayout::p2v(PMT::pte2pa(pmt->m_entries[i]));
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
        pmt->m_entries[i] = 0;
    }
}
