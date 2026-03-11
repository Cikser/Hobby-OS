ARCH := riscv64-unknown-elf
CC := $(ARCH)-gcc
CXX := $(ARCH)-g++
LD := $(ARCH)-ld
OBJCOPY := $(ARCH)-objcopy
OBJDUMP := $(ARCH)-objdump
GDB := gdb-multiarch

TARGET := kernel.elf
KERNEL_DIR := kernel
BUILD_DIR := build
LINKER_SCRIPT := kernel.ld
DISASM_FILE := kernel.asm
GDB_PORT := 1234

# Eksplicitna putanja do startup fajlova
STARTUP_SRC := $(KERNEL_DIR)/start.S
STARTUP_OBJ := $(BUILD_DIR)/start.o

# Pronalaženje svih ostalih izvornih fajlova
SRC_CPP := $(shell find $(KERNEL_DIR) -type f -name "*.cpp")
SRC_C   := $(shell find $(KERNEL_DIR) -type f -name "*.c")
SRC_ASM := $(filter-out $(STARTUP_SRC), $(shell find $(KERNEL_DIR) -type f -name "*.S"))

# Mapiranje objekata
OBJS_CPP := $(patsubst $(KERNEL_DIR)/%.cpp, $(BUILD_DIR)/%.cpp.o, $(SRC_CPP))
OBJS_C   := $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/%.c.o, $(SRC_C))
OBJS_ASM := $(patsubst $(KERNEL_DIR)/%.S, $(BUILD_DIR)/%.S.o, $(SRC_ASM))

# Redosled linkovanja: STARTUP_OBJ prvi
OBJS := $(STARTUP_OBJ) $(OBJS_CPP) $(OBJS_C) $(OBJS_ASM)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(KERNEL_DIR) -type d)
INC_FLAGS := $(addprefix -I, $(INC_DIRS))

CFLAGS := -nostdlib -nostdinc -fno-builtin -fno-stack-protector -mcmodel=medany \
          -march=rv64gc -mabi=lp64d -g $(INC_FLAGS)
CXXFLAGS := $(CFLAGS) -fno-exceptions -fno-rtti -MMD -MP
LDFLAGS := -nostdlib -T $(LINKER_SCRIPT)

.PHONY: all clean qemu qemu-gdb gdb-client disasm

# Dodat disasm u 'all'
all: $(TARGET) disasm

$(TARGET): $(OBJS)
	@$(LD) $(LDFLAGS) -o $@ $^

# --- TARGET ZA DISASEMBLI ---
# Izmeni ovaj deo u Makefile-u:
disasm: $(TARGET)
	@$(OBJDUMP) -d $< > $(DISASM_FILE)

# Specijalni target za startup
$(STARTUP_OBJ): $(STARTUP_SRC)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# Pravila za ostale fajlove
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
	rm -rf $(BUILD_DIR) $(TARGET) $(DISASM_FILE)

qemu: $(TARGET)
	qemu-system-riscv64 -M virt -m 128 -nographic -bios none -kernel $<

qemu-gdb: $(TARGET)
	qemu-system-riscv64 -M virt -m 128 -nographic -bios none -kernel $< -S -s

gdb-client: $(TARGET)
	$(GDB) -ex "target remote localhost:$(GDB_PORT)" $<

-include $(DEPS)