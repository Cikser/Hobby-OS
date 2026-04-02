#ifndef RISC_V_TRAP_H
#define RISC_V_TRAP_H

#include "trapframe.h"

class TrapHandler {
public:
    static void init();
    static void trap();
    static void timerTrap();

    static time_t getTicks() { return s_ticks; };

private:
    static void handleTrap(TrapFrame* trapFrame);

    static time_t s_ticks;

    static constexpr uint64_t SYSCALL = 0x8;
    static constexpr uint64_t PF_INSTRUCTION = 0x12;
    static constexpr uint64_t PF_LOAD = 0x13;
    static constexpr uint64_t PF_STORE = 0x15;
    static constexpr uint64_t TIMER_INTERRUPT = 0x8000000000000001;
    static constexpr uint64_t EXTERNAL_INTERRUPT = 0x8000000000000009;
};

#endif