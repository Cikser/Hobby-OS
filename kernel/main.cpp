#include "test/fstest.h"
#include "fs/vfs.h"
#include "mm/kalloc/kalloc.h"
#include "hw/riscv.h"
#include "io/disk/disk.h"
#include "proc/scheduler.h"
#include "sync/sem.h"
#include "test/memtest.h"
#include "trap/trap.h"

void runTests() {
	MemTest::run();
	//DiskTest::run();
	FsTest::run();
}

void printPid() {
	Console::kprintf("Printing pid: %ld\n", PCB::currentPid());
}

int main() {
    TrapHandler::init();
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
	auto main = new Thread(nullptr);
	RiscV::ms_sstatus(RiscV::SSTATUS_SIE);
	RiscV::ms_sstatus(RiscV::SSTATUS_SPIE);
	auto initProc = Process::createInit();
	auto t1 = new Thread(printPid);

	Semaphore sem(0);

	sem.wait();
	Console::kprintf("After sem\n");

	RiscV::stopEmulation();
}
