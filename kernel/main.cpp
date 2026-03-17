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

int main() {
    TrapHandler::init();
	RiscV::ms_sstatus(1 << 18);
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
    auto* mainPcb = new Thread([]() { while(true) {} });
    mainPcb->m_state = ProcState::RUNNING;
    PCB::s_running = mainPcb;

    Process::createInit();

    PCB::dispatch();
    Console::kprintf("back to main\n");

    while (true) {}
	runTests();
	RiscV::stopEmulation();
}
