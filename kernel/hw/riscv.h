#ifndef RISC_V_RISC_V_H
#define RISC_V_RISC_V_H

#include "../types.h"
#include "mm/vm/vm.h"
#include "trap/trap.h"

class RiscV{

public:
    static void init(int(*main)());
    static void loadSatp(uint64_t pmtAddress);
    static void flushTLB();
    static void w_sstatus(uint64_t value);
    static uint64_t r_sstatus();
    static void w_stvec(uint64_t value);
    static void w_sepc(uint64_t value);
    static uint64_t r_sepc();
    static uint64_t r_scause();
    static uint64_t r_stval();
    static void w_sscratch(uint64_t value);
    static uint64_t r_sscratch();
    static void ms_sstatus(uint64_t mask);
    static void mc_sstatus(uint64_t mask);
    static void ms_sie(uint64_t mask);
    static void mc_sie(uint64_t mask);
    static uint64_t r_sie();
    static void w_sie(uint64_t value);

    static void sModeEntry() __attribute__((section(".text.init")));

    static void stopEmulation(){
        *(volatile uint32_t*)(MemoryLayout::MMIO_BASE + 0x100000ULL) = 0x5555;
        while(true){}
    }

    enum SStatusFlags : uint64_t {
        SSTATUS_SIE = 1 << 1,
        SSTATUS_SPIE = 1 << 5,
        SSTATUS_SPP = 1 << 8
    };

    enum SIEFlags : uint64_t {
        SIE_SEIE = 1 << 9,
        SIE_STIE = 1 << 5,
        SIE_SSIE = 1 << 1
    };

private:
    static void initTimer();
    static uint64_t makeSatp(uint64_t pmtAddress);
    
};

inline void RiscV::initTimer() {
    __asm__ volatile("csrw mtvec, %0" :: "r"((uint64_t)&TrapHandler::timerTrap) : "memory");
    auto mtime = (volatile uint64_t*)0x200BFF8;
    auto mtimecmp = (volatile uint64_t*)0x2004000;
    *mtimecmp = *mtime + 1000000;
    __asm__ volatile("csrs mie, %0" :: "r"(1ULL << 7) : "memory");
}

inline void RiscV::init(int (*main)()) {
    initTimer();
    uint64_t mstatus;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
    mstatus &= ~(3ULL << 11);
    mstatus |=  (1ULL << 11);
    mstatus &= ~(1ULL << 5);
    mstatus &= ~(1ULL << 1);
    __asm__ volatile("csrw mstatus, %0" ::"r"(mstatus));

    __asm__ volatile("csrw mepc, %0"    ::"r"((uint64_t)main));
    __asm__ volatile("csrw medeleg, %0" ::"r"(0xffffULL));
    __asm__ volatile("csrw mideleg, %0" ::"r"(0xffffULL));

    __asm__ volatile("csrw sie, zero");

    __asm__ volatile("csrw pmpaddr0, %0" ::"r"(0x3fffffffffffffULL));
    __asm__ volatile("csrw pmpcfg0, %0"  ::"r"(0xfULL));

    asm volatile("mret");
}


inline uint64_t RiscV::makeSatp(uint64_t pmtAddress){
    uint64_t satp_value = (8ULL << 60) | ((pmtAddress >> 12));
    return satp_value;
}

inline void RiscV::loadSatp(uint64_t pmtAddress){
    uint64_t satp_value = makeSatp(pmtAddress);
    __asm__ volatile("csrw satp, %0" :: "r"(satp_value));
    flushTLB();
}

inline void RiscV::flushTLB(){
    __asm__ volatile("sfence.vma zero, zero");
}

inline void RiscV::w_sstatus(uint64_t value){
    __asm__ volatile("csrw sstatus, %0" :: "r"(value));
}

inline uint64_t RiscV::r_sstatus(){
    uint64_t value;
    __asm__ volatile("csrr %0, sstatus" : "=r"(value));
    return value;
}

inline void RiscV::w_stvec(uint64_t value){
    __asm__ volatile("csrw stvec, %0" :: "r"(value));
}

inline void RiscV::w_sepc(uint64_t value){
    __asm__ volatile("csrw sepc, %0" :: "r"(value));
}

inline uint64_t RiscV::r_sepc(){
    uint64_t value;
    __asm__ volatile("csrr %0, sepc" : "=r"(value));
    return value;
}

inline uint64_t RiscV::r_scause(){
    uint64_t value;
    __asm__ volatile("csrr %0, scause" : "=r"(value));
    return value;
}

inline uint64_t RiscV::r_stval(){
    uint64_t value;
    __asm__ volatile("csrr %0, stval" : "=r"(value));
    return value;
}

inline void RiscV::w_sscratch(uint64_t value) {
    __asm__ volatile("csrw sscratch, %0" :: "r"(value));
}

inline uint64_t RiscV::r_sscratch() {
    uint64_t value;
    __asm__ volatile("csrr %0, sscratch" : "=r"(value));
    return value;
}

inline void RiscV::ms_sstatus(uint64_t mask){
    __asm__ volatile("csrs sstatus, %0" :: "r"(mask));
}

inline void RiscV::mc_sstatus(uint64_t mask){
    __asm__ volatile("csrc sstatus, %0" :: "r"(mask));
}

inline void RiscV::ms_sie(uint64_t mask){
    __asm__ volatile("csrs sie, %0" :: "r"(mask));
}

inline void RiscV::mc_sie(uint64_t mask){
    __asm__ volatile("csrc sie, %0" :: "r"(mask));
}

inline uint64_t RiscV::r_sie(){
    uint64_t value;
    __asm__ volatile("csrr %0, sie" : "=r"(value));
    return value;
}

inline void RiscV::w_sie(uint64_t value){
    __asm__ volatile("csrw sie, %0" :: "r"(value));
}

#endif