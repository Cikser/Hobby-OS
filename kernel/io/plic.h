#ifndef RISC_V_PLIC_H
#define RISC_V_PLIC_H

#include "../types.h"
#include "../hw/memlayout.h"

class PLIC {
public:
    static constexpr uint32_t IRQ_VIRTIO_DISK = 1;
    static constexpr uint32_t IRQ_UART = 10;

    static void setPriority(uint32_t irq, uint32_t priority) {
        *(volatile uint32_t*)(PRIORITY_BASE + irq * 4) = priority;
    }

    static void enableIrq(uint32_t irq) {
        *(volatile uint32_t*)(ENABLE) |= (1u << irq);
    }

    static void setThreshold(uint32_t threshold) {
        *(volatile uint32_t*)(THRESHOLD) = threshold;
    }

    static uint32_t claim() {
        return *(volatile uint32_t*)(CLAIM);
    }

    static void complete(uint32_t irq) {
        *(volatile uint32_t*)(CLAIM) = irq;
    }

private:
    static constexpr uint64_t BASE = MemoryLayout::PLIC_BASE;
    static constexpr uint32_t HART_CONTEXT = 1;
    static constexpr uint64_t ENABLE_STRIDE = 0x80;
    static constexpr uint64_t CONTEXT_STRIDE = 0x1000;

    static constexpr uint64_t PRIORITY_BASE = BASE + 0x0;
    static constexpr uint64_t PENDING_BASE = BASE + 0x1000;
    static constexpr uint64_t ENABLE = BASE + 0x2000 + HART_CONTEXT * ENABLE_STRIDE;
    static constexpr uint64_t THRESHOLD = BASE + 0x200000 + HART_CONTEXT * CONTEXT_STRIDE + 0x0;
    static constexpr uint64_t CLAIM = BASE + 0x200000 + HART_CONTEXT * CONTEXT_STRIDE + 0x4;
};

#endif