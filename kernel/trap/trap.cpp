#include "trap.h"

#include "scheduler.h"
#include "../hw/riscv.h"
#include "../io/plic.h"
#include "../io/console/console.h"
#include "../io/disk/disk.h"
#include "../proc/pcb.h"
#include "syscall/syscall.h"

extern "C" void _trap_kernel_entry();

void TrapHandler::init() {
    RiscV::w_stvec((uint64_t)&_trap_kernel_entry);

    PLIC::setPriority(PLIC::IRQ_VIRTIO_DISK, 1);
    PLIC::enableIrq(PLIC::IRQ_VIRTIO_DISK);
    PLIC::setThreshold(0);

    RiscV::ms_sie(RiscV::SIE_SEIE);
    RiscV::ms_sie(RiscV::SIE_STIE);
    RiscV::ms_sie(RiscV::SIE_SSIE);
}

void TrapHandler::handleTrap(TrapFrame* trapFrame) {
    uint64_t stval = RiscV::r_stval();
    uint64_t scause = RiscV::r_scause();
    uint64_t sepc = RiscV::r_sepc();
    switch (scause) {
        case SYSCALL: {
            trapFrame->sepc += 4;
            SyscallHandler::handle(trapFrame);
            break;
        }
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
        case EXTERNAL_INTERRUPT: {
            uint32_t irq = PLIC::claim();

            if (irq == PLIC::IRQ_VIRTIO_DISK) {
                Disk::interruptHandler();
            }
            if (irq)
                PLIC::complete(irq);
            break;
        }
        default: {
            Console::kprintf("scause: 0x%lx\n", scause);
            Console::kprintf("sepc: 0x%lx\n", sepc);
            Console::kprintf("stval: 0x%lx\n", stval);
            Console::panic("kernel trap");
        }
    }
}