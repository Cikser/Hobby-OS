#include "trap.h"

#include "../hw/riscv.h"
#include "../io/console/console.h"

void TrapHandler::init() {
    RiscV::w_stvec((uint64_t)&trap);
    RiscV::ms_sie(RiscV::SIE_SEIE);
    RiscV::ms_sie(RiscV::SIE_STIE);
}

void TrapHandler::handleTrap(TrapFrame* trapFrame) {
    uint64_t stval = RiscV::r_stval();
    uint64_t scause = RiscV::r_scause();
    uint64_t sepc = RiscV::r_sepc();
    uint64_t sstatus = RiscV::r_sstatus();
    switch (scause) {
        /*case SYSCALL: {
            trapFrame->sepc += 4;
            break;
        }
        case PF_INSTRUCTION: {
            trapFrame->sepc += 4;
            break;
        }
        case PF_LOAD: {
            trapFrame->sepc += 4;
            break;
        }
        case PF_STORE: {
            trapFrame->sepc += 4;
        }*/
        case TIMER_INTERRUPT: {
            Console::kprintf("tick\n");
            break;
        }
        /*case EXTERNAL_INTERRUPT: {
            break;
        }*/
        default: {
            Console::kprintf("scause: 0x%lx\n", scause);
            Console::kprintf("sepc: 0x%lx\n", sepc);
            Console::kprintf("stval: 0x%lx\n", stval);
            Console::panic("kernel trap");
        }
    }
}
