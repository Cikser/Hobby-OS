#include "fstest.h"
#include "fs/vfs.h"
#include "mm/kalloc/kalloc.h"
#include "hw/riscv.h"
#include "io/disk/disk.h"
#include "test/memtest.h"
#include "test/disktest.h"

int main();

extern "C" void start() {
	RiscV::init(main);
}

void runTests() {
	MemTest::run();
	//DiskTest::run();
	FsTest::run();
}

int main() {
	MemoryAllocator::init();
	Disk::init();
	VFS::init();
	runTests();
	RiscV::stopEmulation();
}
