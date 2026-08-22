#include "trap.h"

#include "garbage.h"
#include "scheduler.h"
#include "../hw/riscv.h"
#include "../io/plic.h"
#include "../io/console/console.h"
#include "../io/disk/disk.h"
#include "../proc/pcb.h"
#include "../mm/vm/page_meta.h"
#include "../mm/mem.h"
#include "syscall/syscall.h"

extern "C" void _trap_kernel_entry();

time_t TrapHandler::s_ticks = 0;

void TrapHandler::init() {
    RiscV::w_stvec((uint64_t)&_trap_kernel_entry);

    PLIC::setPriority(PLIC::IRQ_VIRTIO_DISK, 1);
    PLIC::enableIrq(PLIC::IRQ_VIRTIO_DISK);
    PLIC::setPriority(PLIC::IRQ_UART, 1);
    PLIC::enableIrq(PLIC::IRQ_UART);
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
            s_ticks++;
            RiscV::mc_sip(RiscV::SIP_SSIP);
            Scheduler::awake();
            PCBGarbage::clear();
            PCB::s_timeSliceCounter++;
            if (PCB::s_timeSliceCounter >= PCB::running()->m_timeSlice) {
                PCB::yield();
            }
            break;
        }
        case EXTERNAL_INTERRUPT: {
            uint32_t irq = PLIC::claim();
            switch (irq) {
                case PLIC::IRQ_VIRTIO_DISK: {
                    Disk::interruptHandler();
                    break;
                }
                case PLIC::IRQ_UART: {
                    Console::interruptHandler();
                    break;
                }
            }
            if (irq)
                PLIC::complete(irq);
            break;
        }
        case PF_STORE: {
            if (VM::handleCowFault(stval)) break;
            Console::kprintf("store page fault: sepc=0x%lx stval=0x%lx\n", sepc, stval);
            //PCB::runningProcess()->exit(-1);
            //break;
        }
        default: {
            Console::kprintf("scause: 0x%lx\n", scause);
            Console::kprintf("sepc: 0x%lx\n", sepc);
            Console::kprintf("stval: 0x%lx\n", stval);
            Console::kprintf("pid: %d\n", PCB::running()->pid());
            Console::kprintf("mode: %s\n", PCB::running()->m_usermode ? "user" : "kernel");
            Console::panic("kernel trap");
        }
    }
    PCB* running = PCB::running();
    if (running && running->m_usermode) {
        SignalHandler::signalDispatch(trapFrame);
    }
}