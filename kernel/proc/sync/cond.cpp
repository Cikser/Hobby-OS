#include "cond.h"

void Condition::wait() {
    Semaphore::signalWaitAtomic(m_mutex, &m_cond);
}

void Condition::signal() {
    if (m_cond.waiting()) {
        m_cond.signal();
        m_mutex->wait();
    }
}