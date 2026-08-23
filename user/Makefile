ARCH := riscv64-unknown-elf
CC   := $(ARCH)-gcc
LD   := $(ARCH)-ld

ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
LIBC_DIR := $(ROOT_DIR)/libc

MUSL_CC := $(LIBC_DIR)/bin/riscv64-linux-musl-gcc
MUSL_FLAGS := -static -march=rv64gc -mabi=lp64d

CFLAGS := -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
          -march=rv64gc -mabi=lp64d -mcmodel=medany -g

init.elf: init.c init.ld syscall.h
	$(CC) $(CFLAGS) -c init.c -o init.o
	$(LD) -T init.ld -o init.elf init.o

hello_musl.elf: hello_musl.c
	@if [ ! -x "$(MUSL_CC)" ]; then \
		echo "musl toolchain not found at $(MUSL_CC)"; \
		echo "run: ./scripts/install_musl.sh"; \
		exit 1; \
	fi
	$(MUSL_CC) $(MUSL_FLAGS) -o hello_musl.elf hello_musl.c

shell.elf: ../shell/shell
	@cp ../shell/shell shell.elf

clean:
	rm -f *.o *.elf