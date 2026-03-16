ARCH := riscv64-unknown-elf
CC := $(ARCH)-gcc
CXX := $(ARCH)-g++
LD := $(ARCH)-gcc
OBJCOPY := $(ARCH)-objcopy
OBJDUMP := $(ARCH)-objdump
GDB := gdb-multiarch

TARGET := kernel.elf
KERNEL_DIR := kernel
BUILD_DIR := build
LINKER_SCRIPT := kernel.ld
DISASM_FILE := kernel.asm
GDB_PORT := 1234

DISK_IMG := disk.img
DISK_SIZE_MB := 32

STARTUP_SRC := $(KERNEL_DIR)/start.S
STARTUP_OBJ := $(BUILD_DIR)/start.o

SRC_CPP := $(shell find $(KERNEL_DIR) -type f -name "*.cpp")
SRC_C   := $(shell find $(KERNEL_DIR) -type f -name "*.c")
SRC_ASM := $(filter-out $(STARTUP_SRC), $(shell find $(KERNEL_DIR) -type f -name "*.S"))

OBJS_CPP := $(patsubst $(KERNEL_DIR)/%.cpp, $(BUILD_DIR)/%.cpp.o, $(SRC_CPP))
OBJS_C   := $(patsubst $(KERNEL_DIR)/%.c,   $(BUILD_DIR)/%.c.o,   $(SRC_C))
OBJS_ASM := $(patsubst $(KERNEL_DIR)/%.S,   $(BUILD_DIR)/%.S.o,   $(SRC_ASM))

OBJS := $(STARTUP_OBJ) $(OBJS_CPP) $(OBJS_C) $(OBJS_ASM)
DEPS := $(OBJS:.o=.d)

INC_DIRS  := $(shell find $(KERNEL_DIR) -type d)
INC_FLAGS := $(addprefix -I, $(INC_DIRS))

CFLAGS   := -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mcmodel=medany \
            -march=rv64gc -mabi=lp64d -g $(INC_FLAGS)
CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti -MMD -MP -fno-sized-deallocation
LDFLAGS  := -nostdlib -T $(LINKER_SCRIPT) -mcmodel=medany \
                       -march=rv64gc -mabi=lp64d \
                       -Wl,--no-warn-rwx-segments

QEMU_BASE := qemu-system-riscv64 -M virt -m 128 -nographic -bios none \
             -global virtio-mmio.force-legacy=false \
             -kernel $(TARGET)
QEMU_DISK := -drive file=$(DISK_IMG),if=none,format=raw,id=x0 \
             -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

.PHONY: all clean qemu qemu-gdb gdb-client disasm disk-img

all: $(TARGET) $(DISK_IMG) disasm

$(TARGET): $(OBJS)
	@$(LD) $(LDFLAGS) -o $@ $^

disasm: $(TARGET)
	@$(OBJDUMP) -d $< > $(DISASM_FILE)

$(DISK_IMG):
	@echo "Creating disk image: $(DISK_IMG) ($(DISK_SIZE_MB) MB)"
	@dd if=/dev/zero of=$(DISK_IMG) bs=1M count=$(DISK_SIZE_MB) 2>/dev/null
	@mkfs.ext2 -b 1024 -L "kfs" $(DISK_IMG)
	@mkdir -p /tmp/kfs_mnt
	@sudo mount -o loop $(DISK_IMG) /tmp/kfs_mnt
	@echo "hello kernel" | sudo tee /tmp/kfs_mnt/readme.txt    > /dev/null
	@echo "writeable"    | sudo tee /tmp/kfs_mnt/writable.txt  > /dev/null
	@sudo mkdir -p /tmp/kfs_mnt/subdir
	@echo "nested file"  | sudo tee /tmp/kfs_mnt/subdir/nested.txt > /dev/null
	@python3 -c "print('x' * 5000, end='')" | sudo tee /tmp/kfs_mnt/large.txt > /dev/null
	@sudo umount /tmp/kfs_mnt
	@echo "Disk image ready"

disk-img: $(DISK_IMG)

$(STARTUP_OBJ): $(STARTUP_SRC)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.cpp.o: $(KERNEL_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.c.o: $(KERNEL_DIR)/%.c
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.S.o: $(KERNEL_DIR)/%.S
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(DISASM_FILE) $(DISK_IMG)

qemu: $(TARGET) $(DISK_IMG) disasm
	$(QEMU_BASE) $(QEMU_DISK)

qemu-gdb: $(TARGET) $(DISK_IMG)
	$(QEMU_BASE) $(QEMU_DISK) -S -s

gdb-client: $(TARGET)
	$(GDB) -ex "target remote localhost:$(GDB_PORT)" $

-include $(DEPS)