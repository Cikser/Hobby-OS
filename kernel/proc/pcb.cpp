#include "pcb.h"

#include "scheduler.h"
#include "../hw/riscv.h"
#include "../io/console/console.h"
#include "../mm/mem.h"

pid_t PCB::s_pid = 0;
PCB* PCB::s_running = nullptr;

PCB::PCB(uint64_t entry, PMT* pmt, bool usermode) :
    m_pid(s_pid++),
    m_state(ProcState::READY),
    m_pmt(pmt),
    m_next(nullptr),
    m_usermode(usermode),
    m_entry(entry)
{
    m_kstack = (uint8_t*)MemoryAllocator::kallocPages(KERNEL_STACK_SIZE / MemoryLayout::PAGE_SIZE);
    m_trapFrame = (TrapFrame*)(m_kstack + KERNEL_STACK_SIZE - sizeof(TrapFrame));
    m_ustack = usermode ? (uint8_t*)MemoryAllocator::kallocPages(USER_STACK_SIZE / MemoryLayout::PAGE_SIZE) : nullptr;
    memset(&m_context, 0, sizeof(m_context));
    m_context.ra = (uint64_t)pcbEntry;
    m_context.sp = (uint64_t)m_trapFrame;
}

extern "C" void _trap_restore();

void PCB::pcbEntry() {
    PCB* current = s_running;

    if (current->m_usermode) {
        RiscV::ms_sstatus(RiscV::SSTATUS_SPIE);
        RiscV::w_sepc(current->m_entry);
        current->m_trapFrame->sp = USER_STACK_TOP;
        asm volatile(
            "mv sp, %0\n"
            "j  _trap_restore\n"
            :: "r"(current->m_trapFrame)
            : "memory"
        );
    } else {
        ((void(*)())current->m_entry)();
        current->exit();
    }
}

void PCB::exit() {
    m_state = ProcState::ZOMBIE;
    dispatch();
}

void PCB::dispatch() {
    PCB* current = s_running;
    if (current->m_state == ProcState::RUNNING) {
        current->m_state = ProcState::READY;
        Scheduler::put(current);
    }
    PCB* next = Scheduler::get();
    while (next && next->m_state != ProcState::READY)
        next = Scheduler::get();
    if (!next)
        Console::panic("PCB::dispatch(): No ready processes");

    s_running = next;
    next->m_state = ProcState::RUNNING;
    next->m_pmt->activate();

    switchContext(&current->m_context, &next->m_context);
}

PCB::~PCB() {

}
