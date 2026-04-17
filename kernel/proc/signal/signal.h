#ifndef RISC_V_SIGNAL_H
#define RISC_V_SIGNAL_H

#include "../../types.h"
#include "../../trap/trapframe.h"

static constexpr int SIGHUP =  1;
static constexpr int SIGINT =  2;
static constexpr int SIGQUIT =  3;
static constexpr int SIGILL =  4;
static constexpr int SIGTRAP =  5;
static constexpr int SIGABRT =  6;
static constexpr int SIGBUS =  7;
static constexpr int SIGFPE =  8;
static constexpr int SIGKILL =  9;
static constexpr int SIGUSR1 = 10;
static constexpr int SIGSEGV = 11;
static constexpr int SIGUSR2 = 12;
static constexpr int SIGPIPE = 13;
static constexpr int SIGALRM = 14;
static constexpr int SIGTERM = 15;
static constexpr int SIGCHLD = 17;
static constexpr int SIGCONT = 18;
static constexpr int SIGSTOP = 19;
static constexpr int SIGTSTP = 20;
static constexpr int SIGTTIN = 21;
static constexpr int SIGTTOU = 22;
static constexpr int NSIG = 32;

static constexpr uint64_t SIG_DFL = 0;
static constexpr uint64_t SIG_IGN = 1;

static constexpr uint64_t SA_RESTORER = (1ULL << 26);
static constexpr uint64_t SA_RESTART  = (1ULL << 24);
static constexpr uint64_t SA_NOCLDSTOP = (1ULL << 0);

enum class SigDefaultAction : uint8_t {
    TERM,
    IGN,
    CORE,
    STOP,
    CONT,
};

struct SignalAction {
    uint64_t sa_handler;
    uint64_t sa_flags;
    uint64_t sa_restorer;
    uint64_t sa_mask;
};

struct SignalFrame {
    uint64_t signum;
    uint64_t sepc;
    uint64_t ra;
    uint64_t sp;
    uint64_t gp;
    uint64_t tp;
    uint64_t t0, t1, t2;
    uint64_t s0, s1;
    uint64_t a0, a1, a2, a3, a4, a5, a6, a7;
    uint64_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    uint64_t t3, t4, t5, t6;
    uint64_t kstack;
    uint64_t saved_mask;
};

inline SigDefaultAction sigDefaultAction(int signum) {
    switch (signum) {
    case SIGHUP: return SigDefaultAction::TERM;
    case SIGINT: return SigDefaultAction::TERM;
    case SIGQUIT: return SigDefaultAction::CORE;
    case SIGILL: return SigDefaultAction::CORE;
    case SIGTRAP: return SigDefaultAction::CORE;
    case SIGABRT: return SigDefaultAction::CORE;
    case SIGBUS: return SigDefaultAction::CORE;
    case SIGFPE: return SigDefaultAction::CORE;
    case SIGKILL: return SigDefaultAction::TERM;
    case SIGUSR1: return SigDefaultAction::TERM;
    case SIGSEGV: return SigDefaultAction::CORE;
    case SIGUSR2: return SigDefaultAction::TERM;
    case SIGPIPE: return SigDefaultAction::TERM;
    case SIGALRM: return SigDefaultAction::TERM;
    case SIGTERM: return SigDefaultAction::TERM;
    case SIGCHLD: return SigDefaultAction::IGN;
    case SIGCONT: return SigDefaultAction::CONT;
    case SIGSTOP: return SigDefaultAction::STOP;
    case SIGTSTP: return SigDefaultAction::STOP;
    case SIGTTIN: return SigDefaultAction::STOP;
    case SIGTTOU: return SigDefaultAction::STOP;
    default: return SigDefaultAction::TERM;
    }
}

inline bool sigUnblockable(int signum) {
    return signum == SIGKILL || signum == SIGSTOP;
}

inline uint64_t sigBit(int signum) {
    return (signum >= 1 && signum < NSIG) ? (1ULL << (signum - 1)) : 0;
}

#endif