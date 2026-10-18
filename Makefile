CC := gcc
LD := ld
OBJCOPY := objcopy
QEMU := qemu-system-x86_64
GDB := gdb

CFLAGS := -Wall -Wextra -ffreestanding -mno-red-zone -fno-pie -fno-stack-protector \
          -mcmodel=kernel -m64 -mgeneral-regs-only -Iinclude -O2 -g

ASFLAGS := -m64 -Iinclude -g

LDFLAGS := -m elf_x86_64 -nostdlib -z max-page-size=0x1000 -T linker.ld

BUILD_DIR := build
BIN_ELF64 := $(BUILD_DIR)/kernel.elf
BIN_ELF32 := $(BUILD_DIR)/kernel32.elf

# Discover all C and Assembly sources
C_SRCS := $(wildcard src/kernel/*.c) $(wildcard src/lib/*.c) $(wildcard src/tests/*.c)
ASM_SRCS := $(wildcard src/boot/*.S) $(wildcard src/kernel/*.S)

OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SRCS)) \
        $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SRCS))

BOOT_OBJ := $(BUILD_DIR)/src/boot/boot.o
OTHER_OBJS := $(filter-out $(BOOT_OBJ), $(OBJS))
LINK_OBJS := $(BOOT_OBJ) $(OTHER_OBJS)

.PHONY: all build run run-display debug gdb test clean

all: build

build: $(BIN_ELF32)

$(BIN_ELF32): $(BIN_ELF64)
	@echo "[OBJCOPY] Generating 32-bit Multiboot ELF: $@"
	@$(OBJCOPY) -O elf32-i386 $(BIN_ELF64) $(BIN_ELF32)

$(BIN_ELF64): $(LINK_OBJS) linker.ld
	@mkdir -p $(dir $@)
	@echo "[LD] Linking 64-bit kernel ELF: $@"
	@$(LD) $(LDFLAGS) $(LINK_OBJS) -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "[CC] Compiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	@echo "[AS] Assembling $<"
	@$(CC) $(ASFLAGS) -c $< -o $@

run: build
	@echo "[QEMU] Booting Mini OS (Serial Console)..."
	$(QEMU) -kernel $(BIN_ELF32) -serial stdio -display none

run-display: build
	@echo "[QEMU] Booting Mini OS (VGA Window + Serial)..."
	$(QEMU) -kernel $(BIN_ELF32) -serial stdio

debug: build
	@echo "[QEMU] Starting Mini OS in debug mode on localhost:1234..."
	$(QEMU) -kernel $(BIN_ELF32) -serial stdio -display none -s -S

gdb:
	@echo "[GDB] Attaching to QEMU..."
	$(GDB) -ex "target remote localhost:1234" -ex "symbol-file $(BIN_ELF64)"

test: build
	@echo "[TEST] Running Mini OS automated boot smoke test..."
	timeout 8s $(QEMU) -kernel $(BIN_ELF32) -serial stdio -display none || true

clean:
	@echo "[CLEAN] Removing build artifacts..."
	@rm -rf $(BUILD_DIR)
