#include "io/console/console.h"
#include "hw/riscv.h"

int main();

extern "C" void start() {
	RiscV::init(main);
}

int main() {
	Console::kprintf("Hello World!\n");
	RiscV::stopEmulation();
}
