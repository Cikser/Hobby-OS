#include "test/fstest.h"
#include "fs/vfs.h"
#include "mm/kalloc/kalloc.h"
#include "hw/riscv.h"
#include "io/disk/disk.h"
#include "proc/scheduler.h"
#include "test/memtest.h"
#include "trap/trap.h"

void runTests() {
	MemTest::run();
	//DiskTest::run();
	FsTest::run();
}

int main() {
    TrapHandler::init();
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
