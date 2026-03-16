#include "vm.h"
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
    s_bootPmt[level2Index(KERNEL_PHYS)] =
        makePte(KERNEL_PHYS, PAGE_KERN_X);

    s_bootPmt[level2Index(KERNEL_VIRT)] =
        makePte(KERNEL_PHYS, PAGE_KERN_X);

    s_bootPmt[level2Index(PHYS_MAP_VIRT)] =
        makePte(0x00000000ULL, PAGE_KERN);

    s_bootPmt[level2Index(MemoryLayout::MMIO_BASE)] =
        makePte(0x00000000ULL, PAGE_MMIO);
}
