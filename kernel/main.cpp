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

void printPid() {
	Console::kprintf("Printing pid: %ld\n", PCB::currentPid());
}

int main() {
    TrapHandler::init();
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
	RiscV::ms_sstatus(RiscV::SSTATUS_SIE);
	RiscV::ms_sstatus(RiscV::SSTATUS_SPIE);
	RiscV::ms_sie(RiscV::SIE_STIE);
	RiscV::ms_sie(RiscV::SIE_SEIE);
    auto main = new Thread(nullptr);
	auto t1 = new Thread(printPid);
	auto t2 = new Thread(printPid);
	auto initProc = Process::createInit();

	PCB::dispatch();

	RiscV::stopEmulation();
}
