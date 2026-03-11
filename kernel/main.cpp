#include "kalloc.h"
#include "hw/riscv.h"
#include "test/memtest.h"

int main();

extern "C" void start() {
	RiscV::init(main);
}

int main() {
	MemoryAllocator::init();
	MemTest::run();
	RiscV::stopEmulation();
}
