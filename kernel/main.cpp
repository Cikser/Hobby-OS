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

void printPid(void* arg) {
	Console::kprintf("Printing pid: %ld\n", PCB::currentPid());
	((Semaphore*)arg)->signal();
}

int main() {
    TrapHandler::init();
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
	runTests();
	RiscV::stopEmulation();
}
