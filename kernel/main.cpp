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
	DiskTest::run();
}

int main() {
	MemoryAllocator::init();
	Disk::init();
	runTests();
	RiscV::stopEmulation();
}
