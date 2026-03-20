#include "syscall.h"
#include "../../io/console/console.h"
#include "../../proc/pcb.h"
#include "../../fs/file.h"
#include "../../hw/riscv.h"
#include "../../mm/mem.h"

class Process;

void SyscallHandler::handle(TrapFrame* tf) {
    switch (tf->a7) {
    case SYS_EXIT: tf->a0 = sys_exit(tf); break;
    case SYS_GETPID: tf->a0 = sys_getpid(tf); break;
    case SYS_FORK: tf->a0 = sys_fork(tf); break;
    //case SYS_EXECVE:  tf->a0 = sys_execve(tf); break;
    //case SYS_WAIT4: tf->a0 = sys_wait4(tf); break;
    case SYS_READ: tf->a0 = sys_read(tf); break;
    case SYS_WRITE: tf->a0 = sys_write(tf); break;
    case SYS_OPENAT: tf->a0 = sys_openat(tf); break;
    case SYS_CLOSE: tf->a0 = sys_close(tf); break;
    default:
        Console::kprintf("unknown syscall: %d\n", tf->a7);
        tf->a0 = -1;
        break;
    }
}

uint64_t SyscallHandler::sys_getpid(TrapFrame* tf) {
    return PCB::running()->pid();
}

uint64_t SyscallHandler::sys_exit(TrapFrame* tf) {
    PCB::running()->exit();
    return 0;
}

uint64_t SyscallHandler::sys_fork(TrapFrame* tf) {
    PCB* child = PCB::running()->fork();
    return child->pid();
}

uint64_t SyscallHandler::sys_write(TrapFrame* tf) {
    int fd = tf->a0;
    uint64_t buf = tf->a1;
    uint64_t len = tf->a2;

    File* file = PCB::running()->getFile(fd);
    if (!file) return -1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    uint64_t ret = file->write((void*)buf, len);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return ret;
}

uint64_t SyscallHandler::sys_read(TrapFrame* tf) {
    int fd = tf->a0;
    uint64_t buf = tf->a1;
    uint64_t len = tf->a2;

    File* file = PCB::running()->getFile(fd);
    if (!file) return -1;

    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    uint64_t ret = file->read((void*)buf, len);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return ret;
}

uint64_t SyscallHandler::sys_brk(TrapFrame* tf) {
    uint64_t newBrk = tf->a0;
    return PCB::running()->brk(newBrk);
}

uint64_t SyscallHandler::sys_openat(TrapFrame* tf) {
    uint64_t dirfd = tf->a0;
    uint64_t filePath = tf->a1;
    uint64_t flags = tf->a2;
    uint64_t mode = tf->a3;

    char path[256];
    RiscV::ms_sstatus(RiscV::SSTATUS_SUM);
    auto src = (char*)filePath;
    strcpy(path, src);
    RiscV::mc_sstatus(RiscV::SSTATUS_SUM);

    return PCB::running()->openFile(path, flags);
}

uint64_t SyscallHandler::sys_close(TrapFrame* tf) {
    int fd = tf->a0;
    return PCB::running()->closeFile(fd);
}
