#include "lock.h"
#include "../../hw/riscv.h"

KMemCache<Lock>* Lock::s_cache = nullptr;

void Lock::acquire() {
    m_pie = RiscV::r_sstatus() & RiscV::SSTATUS_SIE;
    RiscV::mc_sstatus(RiscV::SSTATUS_SIE);
    while (__sync_lock_test_and_set(&m_locked, 1));
}

void Lock::release() {
    __sync_lock_release(&m_locked);
    RiscV::ms_sstatus(RiscV::SSTATUS_SIE & m_pie);
}
