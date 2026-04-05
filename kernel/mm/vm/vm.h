#ifndef RISC_V_VM_H
#define RISC_V_VM_H

#include "mmap.h"
#include "pmt.h"
#include "../../types.h"

class VM {
public:
    static void bootstrap() __attribute__((section(".text.init")));

    static PMT* createPMT();
    static void destroyPMT(const PMT* pmt);

    static bool copyPMT(PMT* dst, PMT* src, const Mmap* skipMmap = nullptr);
    static void clearUserPages(PMT* pmt);

private:
    friend class Thread;
    alignas(4096) static uint64_t s_bootPmt[PMT::PMT_SIZE];

    static constexpr uint32_t USER_THRESHOLD = 256;
    static constexpr uint64_t level2Index(uint64_t va);
};

constexpr uint64_t VM::level2Index(uint64_t va) {
    return (va >> 30) & 0x1FF;
}

#endif