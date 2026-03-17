# userspace/Makefile
ARCH := riscv64-unknown-elf
CC   := $(ARCH)-gcc
LD   := $(ARCH)-ld

CFLAGS := -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
          -march=rv64gc -mabi=lp64d -mcmodel=medany -g

init.elf: init.c init.ld
	$(CC) $(CFLAGS) -c init.c -o init.o
	$(LD) -T init.ld -o init.elf init.o

clean:
	rm -f init.o init.elf