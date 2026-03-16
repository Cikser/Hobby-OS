#include "test/fstest.h"
#include "fs/vfs.h"
#include "mm/kalloc/kalloc.h"
#include "hw/riscv.h"
#include "io/disk/disk.h"
#include "proc/scheduler.h"
#include "test/memtest.h"
#include "trap/trap.h"
#include "vm/vm.h"

int main();

extern "C" void start() {
    VM::init();
	RiscV::init(main);
}

void runTests() {
	MemTest::run();
	//DiskTest::run();
	FsTest::run();
}

void threadFunc() {
    Console::kprintf("hello from thread %ld\n", PCB::s_running->pid());
    PCB::s_running->exit();
}

int main() {
    TrapHandler::init();
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
	//runTests();
    auto* mainPcb = new Thread(nullptr);
    mainPcb->m_state = ProcState::RUNNING;
    PCB::s_running = mainPcb;

    auto* t1 = new Thread(threadFunc);
    auto* t2 = new Thread(threadFunc);
    Scheduler::put(t1);
    Scheduler::put(t2);
    PCB::dispatch();
	RiscV::stopEmulation();
}
