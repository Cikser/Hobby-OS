#include "fs/vfs.h"
#include "mm/kalloc/kalloc.h"
#include "hw/riscv.h"
#include "io/disk/disk.h"
#include "trap/trap.h"
#include "proc/process/process.h"
#include "proc/thread/thread.h"
#include "io/console/console.h"
#include "io/disk/block_cache.h"

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
	Console::init();
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
	auto main = new Thread(nullptr);
	Process* initProc = Process::createInit("/bin/sh");
	Disk::enableInterruptMode();
	RiscV::ms_sstatus(RiscV::SSTATUS_SIE);
	RiscV::ms_sstatus(RiscV::SSTATUS_SPIE);

	while (initProc->state() != ProcState::ZOMBIE);
	BlockCache::flush();
	Console::kprintf("back in main\n");

	RiscV::stopEmulation();
}
