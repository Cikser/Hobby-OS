#include "test/fstest.h"
#include "fs/vfs.h"
#include "mm/kalloc/kalloc.h"
#include "hw/riscv.h"
#include "io/disk/disk.h"
#include "proc/scheduler.h"
#include "sync/sem.h"
#include "test/memtest.h"
#include "trap/trap.h"
#include "proc/process/process.h"
#include "proc/thread/thread.h"

void runTests(void* arg) {
	MemTest::run();
	//DiskTest::run();
	FsTest::run();
	((Semaphore*)arg)->signal();
}

void printPid(void* arg) {
	Console::kprintf("Printing pid: %ld\n", PCB::currentPid());
}

void printSleep(void* arg) {
	Console::kprintf("Sleeping pid: %ld\n", PCB::currentPid());
	PCB::sleep(1000);
	Console::kprintf("Finished sleeping pid: %ld\n", PCB::currentPid());
}

int main() {
    TrapHandler::init();
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
	auto main = new Thread(nullptr);
	Process* initProc = Process::createInit();
	RiscV::ms_sstatus(RiscV::SSTATUS_SIE);
	RiscV::ms_sstatus(RiscV::SSTATUS_SPIE);

	auto t1 = new Thread(printSleep);
	auto t2 = new Thread(printPid);

	while (initProc->state() != ProcState::ZOMBIE || t1->state() != ProcState::ZOMBIE
		|| t2->state() != ProcState::ZOMBIE);

	Console::kprintf("back in main\n");

	RiscV::stopEmulation();
}
