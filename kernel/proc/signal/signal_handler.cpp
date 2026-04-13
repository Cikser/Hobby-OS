#include "signal_handler.h"
#include "signal.h"
#include "../../proc/pcb.h"
#include "../../proc/process/process.h"
#include "../../mm/mem.h"
#include "../../hw/riscv.h"
#include "../../io/console/console.h"

KMemCache<SignalHandler>* SignalHandler::s_cache = nullptr;

SignalHandler::SignalHandler()
    : m_refCount(1), m_pending(0), m_defaultMask(0)
{
    for (int i = 0; i < NSIG; i++) {
        m_actions[i] = { SIG_DFL, 0, 0, 0 };
    }
}

SignalHandler* SignalHandler::clone() const {
    m_lock.acquire();
    auto* copy = new SignalHandler();
    for (int i = 0; i < NSIG; i++)
        copy->m_actions[i] = m_actions[i];
    copy->m_defaultMask = m_defaultMask;
    copy->m_pending = 0;
    m_lock.release();
    return copy;
}

void SignalHandler::send(int signum) {
    if (signum < 1 || signum >= NSIG) return;

    m_lock.acquire();

    if (!sigUnblockable(signum) &&
        m_actions[signum].sa_handler == SIG_IGN) {
        m_lock.release();
        return;
    }

    m_pending |= sigBit(signum);
    m_lock.release();
}

bool SignalHandler::hasPending(uint64_t threadMask) const {
    m_lock.acquire();
    bool has = (m_pending & ~threadMask) != 0;
    m_lock.release();
    return has;
}

int SignalHandler::dequeue(uint64_t threadMask) {
    m_lock.acquire();
    uint64_t deliverable = m_pending & ~threadMask;
    if (!deliverable) {
        m_lock.release();
        return 0;
    }

    int bit = 0;
    while (bit < NSIG - 1 && !(deliverable & (1ULL << bit)))
        bit++;
    m_pending &= ~(1ULL << bit);
    m_lock.release();
    return bit + 1;
}

int SignalHandler::setAction(int signum, const SignalAction* act, SignalAction* oldact) {
    if (signum < 1 || signum >= NSIG) return -1;
    if (sigUnblockable(signum) && act) return -1;

    m_lock.acquire();
    if (oldact)
        *oldact = m_actions[signum];
    if (act) {
        m_actions[signum] = *act;
        m_actions[signum].sa_mask &= ~(sigBit(SIGKILL) | sigBit(SIGSTOP));
    }
    m_lock.release();
    return 0;
}

void SignalHandler::getAction(int signum, SignalAction* act) const {
    if (!act || signum < 1 || signum >= NSIG) return;
    m_lock.acquire();
    *act = m_actions[signum];
    m_lock.release();
}

static bool pushSignalFrame(uint64_t* userSp, const SignalFrame& frame) {
    uint64_t sp = *userSp;
    sp -= sizeof(SignalFrame);
    sp &= ~(uint64_t)15;

    Process* proc = PCB::runningProcess();
    if (!proc->checkOperation(sp, sizeof(SignalFrame), SegmentDesc::SEG_W))
        return false;

    memcpy((void*)sp, &frame, sizeof(SignalFrame));
    *userSp = sp;
    return true;
}

static bool executeDefault(int signum, TrapFrame* tf) {
    SigDefaultAction action = sigDefaultAction(signum);
    Process* proc = PCB::runningProcess();

    switch (action) {
        case SigDefaultAction::IGN:
            return true;

        case SigDefaultAction::TERM:
        case SigDefaultAction::CORE:
            proc->exit(-(signum));
            return false;

        case SigDefaultAction::STOP:
            PCB::running()->setState(ProcState::BLOCKED);
            PCB::yield();
            return true;

        case SigDefaultAction::CONT:
            return true;
    }
    return true;
}


void SignalHandler::signalDispatch(TrapFrame* tf) {
    Process* proc = PCB::runningProcess();
    if (!proc) return;

    SignalHandler* sh = proc->m_signalHandler;
    if (!sh) return;

    uint64_t threadMask = PCB::running()->m_sigMask;

    int signum = sh->dequeue(threadMask);
    if (!signum) return;

    SignalAction action;
    sh->getAction(signum, &action);

    if (action.sa_handler == SIG_DFL) {
        executeDefault(signum, tf);
        return;
    }

    if (action.sa_handler == SIG_IGN) {
        return;
    }

    SignalFrame frame;
    frame.signum = (uint64_t)signum;
    frame.sepc = tf->sepc;
    frame.ra = tf->ra;
    frame.sp = tf->sp;
    frame.gp = tf->gp;
    frame.tp = tf->tp;
    frame.t0 = tf->t0;
    frame.t1 = tf->t1;
    frame.t2 = tf->t2;
    frame.s0 = tf->s0;
    frame.s1 = tf->s1;
    frame.a0 = tf->a0;
    frame.a1 = tf->a1;
    frame.a2 = tf->a2;
    frame.a3 = tf->a3;
    frame.a4 = tf->a4;
    frame.a5 = tf->a5;
    frame.a6 = tf->a6;
    frame.a7 = tf->a7;
    frame.s2 = tf->s2;
    frame.s3 = tf->s3;
    frame.s4 = tf->s4;
    frame.s5 = tf->s5;
    frame.s6 = tf->s6;
    frame.s7 = tf->s7;
    frame.s8 = tf->s8;
    frame.s9 = tf->s9;
    frame.s10 = tf->s10;
    frame.s11 = tf->s11;
    frame.t3 = tf->t3;
    frame.t4 = tf->t4;
    frame.t5 = tf->t5;
    frame.t6 = tf->t6;
    frame.kstack = tf->kstack;
    frame.saved_mask = threadMask;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    uint64_t newSp = tf->sp;
    bool ok = pushSignalFrame(&newSp, frame);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    if (!ok) {
        Console::kprintf("signal: stack overflow delivering sig %d, killing\n", signum);
        Process* p = PCB::runningProcess();
        if (p) p->exit(-SIGSEGV);
        return;
    }

    PCB::running()->m_sigMask = threadMask | (action.sa_mask & ~(sigBit(SIGKILL) | sigBit(SIGSTOP)));

    tf->sp   = newSp;
    tf->a0   = (uint64_t)signum;
    tf->ra   = action.sa_restorer;
    tf->sepc = action.sa_handler;
}