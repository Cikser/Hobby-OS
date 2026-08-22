#ifndef RISC_V_SIGNAL_HANDLER_H
#define RISC_V_SIGNAL_HANDLER_H

#include "signal.h"
#include "../../types.h"
#include "../../mm/kalloc/kmem_cache.h"
#include "../sync/lock.h"

class SignalHandler {
public:
    SignalHandler();
    ~SignalHandler() = default;

    static void signalDispatch(TrapFrame* tf);

    void acquire() {
        m_lock.acquire();
        m_refCount++;
        m_lock.release();
    }

    bool release() {
        m_lock.acquire();
        bool last = (--m_refCount == 0);
        m_lock.release();
        return last;
    }

    SignalHandler* clone() const;
    void send(int signum);
    bool hasPending(uint64_t threadMask) const;
    int dequeue(uint64_t threadMask);

    int setAction(int signum, const SignalAction* act, SignalAction* oldact);
    void getAction(int signum, SignalAction* act) const;

    uint64_t defaultMask() const {
        m_lock.acquire();
        uint64_t m = m_defaultMask;
        m_lock.release();
        return m;
    }

    void resetForExec() {
        m_lock.acquire();
        for (int i = 0; i < NSIG; i++) {
            if (m_actions[i].sa_handler != SIG_IGN) {
                m_actions[i] = { SIG_DFL, 0, 0, 0 };
            }
        }
        m_lock.release();
    }

    void* operator new(size_t size) {
        if (!s_cache) s_cache = new KMemCache<SignalHandler>();
        return s_cache->alloc();
    }
    void operator delete(void* ptr) { s_cache->free(ptr); }

private:
    mutable Lock m_lock;
    uint32_t m_refCount;
    uint64_t m_pending;
    uint64_t m_defaultMask;
    SignalAction m_actions[NSIG];

    static KMemCache<SignalHandler>* s_cache;

    bool hasPendingRaw(uint64_t bits) const {
        m_lock.acquire();
        bool has = (m_pending & bits) != 0;
        m_lock.release();
        return has;
    }
};

#endif