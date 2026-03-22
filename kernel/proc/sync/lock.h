#ifndef RISC_V_LOCK_H
#define RISC_V_LOCK_H

#include "../../types.h"

class Lock {
public:
    Lock() : m_locked(0), m_pie(0) {};

    void acquire();
    void release();

private:
    uint32_t m_locked;
    uint32_t m_pie;

};

#endif