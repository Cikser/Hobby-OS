#ifndef RISC_V_RISC_V_H
#define RISC_V_RISC_V_H

#include "../types.h"
#include "mm/vm/vm.h"
#include "trap/trap.h"

class RiscV{

public:
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
    static void ms_sip(uint64_t mask);
    static void mc_sip(uint64_t mask);
    static uint64_t r_sip();
    static void w_sip(uint64_t value);

    static void sModeEntry() __attribute__((section(".text.init")));

    static void stopEmulation(){
        *(volatile uint32_t*)(MemoryLayout::MMIO_BASE + 0x100000ULL) = 0x5555;
        while(true){}
    }

    enum SStatusFlags : uint64_t {
        SSTATUS_SIE = 1 << 1,
        SSTATUS_SPIE = 1 << 5,
        SSTATUS_SPP = 1 << 8,
        SSTATUS_SUM = 1 << 18
    };

    enum SIEFlags : uint64_t {
        SIE_SEIE = 1 << 9,
        SIE_STIE = 1 << 5,
        SIE_SSIE = 1 << 1
    };

    enum SIPFlags : uint64_t {
        SIP_SSIP = 1 << 1,
        SIP_STIP = 1 << 5,
        SIP_SEIP = 1 << 9,
    };

private:
    static uint64_t makeSatp(uint64_t pmtAddress);
    
};

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

inline uint64_t RiscV::r_sip() {
    uint64_t value;
    __asm__ volatile("csrr %0, sip" : "=r"(value));
    return value;
}

inline void RiscV::w_sip(uint64_t value) {
    __asm__ volatile("csrw sip, %0" :: "r"(value));
}

inline void RiscV::mc_sip(uint64_t mask) {
    __asm__ volatile("csrc sip, %0" :: "r"(mask));
}

inline void RiscV::ms_sip(uint64_t mask) {
    __asm__ volatile("csrs sip, %0" :: "r"(mask));
}

#endif