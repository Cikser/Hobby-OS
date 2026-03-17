#include "riscv.h"
#include "memlayout.h"

extern "C" char _boot_pmt_pa[];
extern "C" char _main_pa[];

extern "C" void s_mode_entry() __attribute__((section(".text.init")));
extern "C" void s_mode_entry() { RiscV::sModeEntry(); }

void RiscV::sModeEntry() {
    constexpr uint64_t SATP_MODE_SV39 = 8ULL << 60;

    uint64_t boot_pa = (uint64_t)_boot_pmt_pa;
    uint64_t satp = SATP_MODE_SV39 | (boot_pa >> 12);
    __asm__ volatile("csrw satp, %0" :: "r"(satp) : "memory");
    __asm__ volatile("sfence.vma zero, zero" ::: "memory");

    __asm__ volatile(
        "la sp, boot_stack_top\n"
        "li t0, 0xFFFFFFC000000000\n"
        "add sp, sp, t0\n"
        ::: "memory"
    );

    uint64_t main_va = (uint64_t)_main_pa + MemoryLayout::KERNEL_OFFSET;
    ((void (*)())main_va)();

    while (true) {}
}

