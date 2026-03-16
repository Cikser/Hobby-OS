#include "test/fstest.h"
#include "fs/vfs.h"
#include "mm/kalloc/kalloc.h"
#include "hw/riscv.h"
#include "io/disk/disk.h"
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
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
	runTests();
	RiscV::stopEmulation();
}
