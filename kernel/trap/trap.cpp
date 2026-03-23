#include "trap.h"

#include "scheduler.h"
#include "../hw/riscv.h"
#include "../io/console/console.h"
#include "../proc/pcb.h"
#include "syscall/syscall.h"

extern "C" void _trap_kernel_entry();

void TrapHandler::init() {
    RiscV::w_stvec((uint64_t)&_trap_kernel_entry);
    RiscV::ms_sie(RiscV::SIE_SEIE);
    RiscV::ms_sie(RiscV::SIE_STIE);
    RiscV::ms_sie(RiscV::SIE_SSIE);
}

void TrapHandler::handleTrap(TrapFrame* trapFrame) {
    // Keep trap handling non-preemptible; execution mode policy is restored on exit.
    RiscV::mc_sstatus(RiscV::SSTATUS_SIE);
    if (RiscV::r_sstatus() & RiscV::SSTATUS_SIE) {
        Console::panic("TrapHandler::handleTrap(): interrupts must be disabled");
    }
    uint64_t stval = RiscV::r_stval();
    uint64_t scause = RiscV::r_scause();
    uint64_t sepc = RiscV::r_sepc();
    switch (scause) {
        case SYSCALL: {
            trapFrame->sepc += 4;
            SyscallHandler::handle(trapFrame);
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
            RiscV::mc_sip(RiscV::SIP_SSIP);
            Scheduler::awake();
            PCB::s_timeSliceCounter++;
            if (PCB::s_timeSliceCounter >= PCB::running()->m_timeSlice) {
                uint64_t sstatus = RiscV::r_sstatus();
                PCB::dispatch();
                RiscV::w_sstatus(sstatus);
            }
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
