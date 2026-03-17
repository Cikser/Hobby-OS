#include "trap.h"

#include "../hw/riscv.h"
#include "../io/console/console.h"
#include "../proc/pcb.h"

extern "C" void _trap_kernel_entry();

void TrapHandler::init() {
    RiscV::w_stvec((uint64_t)&_trap_kernel_entry);
    RiscV::ms_sie(RiscV::SIE_SEIE);
    RiscV::ms_sie(RiscV::SIE_STIE);
    RiscV::ms_sie(RiscV::SIE_SSIE);
}

void TrapHandler::handleTrap(TrapFrame* trapFrame) {
    uint64_t stval = RiscV::r_stval();
    uint64_t scause = RiscV::r_scause();
    uint64_t sepc = RiscV::r_sepc();
    uint64_t sstatus = RiscV::r_sstatus();
    switch (scause) {
        case SYSCALL: {
            trapFrame->sepc += 4;
            if (trapFrame->a7 == 1) {
                Console::kprintf("Hello world!\n");
            }
            PCB::dispatch();
            break;
        }/*
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
            PCB::dispatch();
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
