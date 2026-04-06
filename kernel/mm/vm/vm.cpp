#include "vm.h"
#include "../mem.h"
#include "pmt.h"
#include "page_meta.h"
#include "../../hw/memlayout.h"
#include "../../hw/riscv.h"
#include "../../proc/process/process.h"

alignas(4096) uint64_t VM::s_bootPmt[PMT::PMT_SIZE];

extern "C" char _bss_start_pa[];
extern "C" char _bss_end_pa[];
extern "C" char _boot_pmt_pa[];

void VM::bootstrap() {
    constexpr uint64_t KERNEL_PHYS   = MemoryLayout::PHYS_BASE;
    constexpr uint64_t PHYS_MAP_VIRT = MemoryLayout::KERNEL_OFFSET;

    auto* bss_cur = (uint64_t*)_bss_start_pa;
    auto* bss_end = (uint64_t*)_bss_end_pa;
    while (bss_cur < bss_end) *bss_cur++ = 0;

    auto* boot = (uint64_t*)_boot_pmt_pa;
    for (int i = 0; i < PMT::PMT_SIZE; i++) boot[i] = 0;

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

    for (int i = USER_THRESHOLD; i < PMT::PMT_SIZE; i++) {
        pmt->m_entries[i] = s_bootPmt[i];
    }
    return pmt;
}

void VM::destroyPMT(const PMT* pmt) {
    if (!pmt) return;

    for (int i = 0; i < USER_THRESHOLD; i++) {
        uint64_t l2pte = pmt->m_entries[i];
        if (!PMT::pteValid(l2pte)) continue;

        auto* l1 = (PMT*)MemoryLayout::p2v(PMT::pte2pa(l2pte));
        for (auto l1pte : l1->m_entries) {
            if (!PMT::pteValid(l1pte)) continue;

            auto* l0 = (PMT*)MemoryLayout::p2v(PMT::pte2pa(l1pte));
            for (auto l0pte : l0->m_entries) {
                if (!PMT::pteValid(l0pte)) continue;
                uint64_t pa = PMT::pte2pa(l0pte);
                if (PageRefCount::decRef(pa))
                    MemoryAllocator::kfreePage((void*)MemoryLayout::p2v(pa));
            }
            delete l0;
        }
        delete l1;
    }
    delete pmt;
}

bool VM::copyPMT(PMT* dst, PMT* src, const Mmap* skipMmap) {
    PageRefCount::init();
    for (int i = 0; i < USER_THRESHOLD; i++) {
        if (!PMT::pteValid(src->m_entries[i])) continue;

        auto* l1src = (PMT*)PMT::pte2table(src->m_entries[i]);
        auto* l1dst = new PMT();
        if (!l1dst) return false;

        bool l1_has_entry = false;

        for (int j = 0; j < PMT::PMT_SIZE; j++) {
            if (!PMT::pteValid(l1src->m_entries[j])) continue;

            auto* l0src = (PMT*)PMT::pte2table(l1src->m_entries[j]);
            auto* l0dst = new PMT();
            if (!l0dst) {
                delete l1dst;
                return false;
            }

            bool l0_has_entry = false;

            for (int k = 0; k < PMT::PMT_SIZE; k++) {
                uint64_t srcPte = l0src->m_entries[k];
                if (!PMT::pteValid(srcPte)) continue;

                uint64_t va = ((uint64_t)i << PMT::L2_OFFSET) |
                              ((uint64_t)j << PMT::L1_OFFSET) |
                              ((uint64_t)k << PMT::L0_OFFSET);

                if (skipMmap && skipMmap->find(va)) continue;

                uint64_t pa = PMT::pte2pa(srcPte);
                uint64_t flags = srcPte & 0x3FF;

                if (flags & PMT::PAGE_W) {
                    uint64_t cowFlags = (flags & ~PMT::PAGE_W) | PMT::PAGE_COW;
                    l0src->m_entries[k] = PMT::makePte(pa, cowFlags);
                    l0dst->m_entries[k] = PMT::makePte(pa, cowFlags);
                }
                else {
                    l0dst->m_entries[k] = srcPte;
                }
                PageRefCount::incRef(pa);
                l0_has_entry = true;
            }

            if (!l0_has_entry) {
                delete l0dst;
                continue;
            }

            l1dst->m_entries[j] = PMT::makePte(MemoryLayout::v2p((uint64_t)l0dst), PMT::PAGE_V);
            l1_has_entry = true;
        }

        if (!l1_has_entry) {
            delete l1dst;
            continue;
        }

        dst->m_entries[i] = PMT::makePte(MemoryLayout::v2p((uint64_t)l1dst), PMT::PAGE_V);
    }

    RiscV::flushTLB();
    return true;
}

void VM::clearUserPages(PMT* pmt) {
    for (int i = 0; i < USER_THRESHOLD; i++) {
        if (!PMT::pteValid(pmt->m_entries[i])) continue;

        auto* l1 = (PMT*)MemoryLayout::p2v(PMT::pte2pa(pmt->m_entries[i]));
        for (auto l1pte : l1->m_entries) {
            if (!PMT::pteValid(l1pte)) continue;

            auto* l0 = (PMT*)MemoryLayout::p2v(PMT::pte2pa(l1pte));
            for (auto l0pte : l0->m_entries) {
                if (!PMT::pteValid(l0pte)) continue;
                uint64_t pa = PMT::pte2pa(l0pte);
                if (PageRefCount::decRef(pa))
                    MemoryAllocator::kfreePage((void*)MemoryLayout::p2v(pa));
            }
            delete l0;
        }
        delete l1;
        pmt->m_entries[i] = 0;
    }
}

bool VM::handleCowFault(uint64_t faultVa) {
    Process* proc = PCB::runningProcess();
    if (!proc) return false;

    PMT* pmt = proc->pmt();
    if (!pmt) return false;

    uint64_t flags = pmt->getFlags(faultVa);
    if (!(flags & PMT::PAGE_COW)) return false;

    uint64_t va = MemoryLayout::pageRoundDown(faultVa);
    uint64_t pa = pmt->translate(va);
    if (!pa) return false;

    uint64_t newFlags = (flags & ~PMT::PAGE_COW) | PMT::PAGE_W;
    uint32_t ref = PageRefCount::getRef(pa);

    if (ref <= 1) {
        pmt->unmapPage(va);
        pmt->mapPage(va, pa, newFlags);
        PageRefCount::decRef(pa);
    }
    else {
        void* newPage = MemoryAllocator::kallocPage();
        if (!newPage) return false;

        memcpy(newPage, (void*)MemoryLayout::p2v(pa), MemoryLayout::PAGE_SIZE);

        uint64_t newPa = MemoryLayout::v2p((uint64_t)newPage);

        pmt->unmapPage(va);
        pmt->mapPage(va, newPa, newFlags);

        if (PageRefCount::decRef(pa)) {
            MemoryAllocator::kfreePage((void*)MemoryLayout::p2v(pa));
        }
    }

    RiscV::flushTLB();
    return true;
}
