ARCH := riscv64-unknown-elf
CC   := $(ARCH)-gcc
LD   := $(ARCH)-ld

MUSL_CC := $(HOME)/CLionProjects/musl/bin/riscv64-linux-musl-gcc
MUSL_FLAGS := -static -march=rv64gc -mabi=lp64d

CFLAGS := -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
          -march=rv64gc -mabi=lp64d -mcmodel=medany -g

init.elf: init.c init.ld syscall.h
	$(CC) $(CFLAGS) -c init.c -o init.o
	$(LD) -T init.ld -o init.elf init.o

hello_musl: hello_musl.c
	$(MUSL_CC) $(MUSL_FLAGS) -o hello_musl hello_musl.c

clean:
	rm -f init.o init.elf hello_musl