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
	((Semaphore*)arg)->signal();
}

void printSleep(void* arg) {
	Console::kprintf("Sleeping pid: %ld\n", PCB::currentPid());
	PCB::sleep(PCB::running()->pid() * 1000);
	Console::kprintf("Finished sleeping pid: %ld\n", PCB::currentPid());
}

int main() {
    TrapHandler::init();
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
	auto main = new Thread(nullptr);
	RiscV::ms_sstatus(RiscV::SSTATUS_SIE);
	RiscV::ms_sstatus(RiscV::SSTATUS_SPIE);
	//Process* initProc = Process::createInit();

	Thread* threads[5];
	for (int i = 0; i < 5; i++) {
		threads[i] = new Thread(printSleep);
	}


	while (true) {
		if (threads[0]->state() == ProcState::ZOMBIE &&
			threads[1]->state() == ProcState::ZOMBIE &&
			threads[2]->state() == ProcState::ZOMBIE &&
			threads[3]->state() == ProcState::ZOMBIE &&
			threads[4]->state() == ProcState::ZOMBIE) {
			RiscV::stopEmulation();
		}
	}
	//while (!Scheduler::empty()) PCB::dispatch();

	Console::kprintf("back in main\n");

	RiscV::stopEmulation();
}
