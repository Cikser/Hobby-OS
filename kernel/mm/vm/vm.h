#ifndef RISC_V_VM_H
#define RISC_V_VM_H

#include "pmt.h"
#include "../../types.h"

class VM {
public:
    static void bootstrap() __attribute__((section(".text.init")));

    static PMT* createPMT();
    static void destroyPMT(const PMT* pmt);

    static bool copyPMT(PMT* dst, PMT* src);
    static void clearUserPages(PMT* pmt);

private:
    friend class Thread;
    alignas(4096) static uint64_t s_bootPmt[512];

    static constexpr uint64_t level2Index(uint64_t va);
};

constexpr uint64_t VM::level2Index(uint64_t va) {
    return (va >> 30) & 0x1FF;
}

#endif