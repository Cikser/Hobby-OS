#include "vm.h"
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