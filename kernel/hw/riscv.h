#ifndef __RISC_V_H__
#define __RISC_V_H__

#include "../types.h"

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
    static void ms_sstatus(uint64_t mask);
    static void mc_sstatus(uint64_t mask);
    static void ms_sie(uint64_t mask);
    static void mc_sie(uint64_t mask);
    static uint64_t r_sie();
    static void w_sie(uint64_t value);

    static void stopEmulation(){
        __asm__ volatile("li t0, 0x100000");
        __asm__ volatile("li t1, 0x5555");
        __asm__ volatile("sw t1, 0(t0)");
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

    static void trap();

private:
    static void trapHandler();
    static uint64_t makeSatp(uint64_t pmtAddress);
    
};

inline void RiscV::init(int (*main)()) {
    unsigned long mstatus;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
    mstatus &= ~(3 << 11);
    mstatus |= (1 << 11);
    __asm__ volatile("csrw mstatus, %0" ::"r"(mstatus));

    __asm__ volatile("csrw mepc, %0" ::"r"((uint64_t)main));

    __asm__ volatile("csrw satp, zero");

    __asm__ volatile("csrw medeleg, %0" ::"r"(0xffff));
    __asm__ volatile("csrw mideleg, %0" ::"r"(0xffff));

    w_sie(r_sie() | SIE_SEIE | SIE_STIE);

    __asm__ volatile("csrw pmpaddr0, %0" ::"r"(0x3fffffffffffffull));
    __asm__ volatile("csrw pmpcfg0, %0" ::"r"(0xf));

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