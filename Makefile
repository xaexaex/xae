# ==============================================================================
# XAE OS MAKEFILE
# ==============================================================================
# WHAT: Build system that compiles everything into a bootable OS image
# WHY: Automates the complex process of building an OS
# HOW: Assembles bootloader, compiles kernel, links everything together

# Compiler and tools
ASM = nasm
CC = gcc
LD = ld

# Compiler flags
# -m32: Build for 32-bit x86
# -ffreestanding: We're not using standard library
# -nostdlib: Don't link standard library
# -fno-pie: Don't create position-independent executable
# -Wall: Show all warnings
CFLAGS = -m32 -ffreestanding -nostdlib -fno-pie -Wall -Wextra -O2 -I kernel
LDFLAGS = -m elf_i386 -T linker.ld
ASMFLAGS = -f elf32

# Source files
BOOT_SRC = boot/boot.asm
KERNEL_ENTRY = kernel/entry.asm
KERNEL_C_SRC = $(wildcard kernel/*.c kernel/drivers/*.c kernel/mm/*.c kernel/fs/*.c kernel/lib/*.c)

# Object files
KERNEL_ENTRY_OBJ = build/entry.o
KERNEL_C_OBJ = $(patsubst %.c, build/%.o, $(notdir $(KERNEL_C_SRC)))

# Output files
BOOTLOADER = build/boot.bin
KERNEL = build/kernel.bin
OS_IMAGE = build/xae_os.img

# Default target
all: $(OS_IMAGE)
	@echo "===================================="
	@echo "XAE OS built successfully!"
	@echo "Run with: make run"
	@echo "===================================="

# Build the final OS image
$(OS_IMAGE): $(BOOTLOADER) $(KERNEL)
	@echo "Creating OS image..."
	cat $(BOOTLOADER) $(KERNEL) > $(OS_IMAGE)
	# Pad to 1.44MB (floppy size)
	truncate -s 1440K $(OS_IMAGE) 2>/dev/null || dd if=/dev/zero bs=1024 count=1440 of=$(OS_IMAGE) 2>/dev/null

# Build bootloader (raw binary)
$(BOOTLOADER): $(BOOT_SRC)
	@mkdir -p build
	@echo "Assembling bootloader..."
	$(ASM) -f bin $(BOOT_SRC) -o $(BOOTLOADER)

# Build kernel
$(KERNEL): $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJ)
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o $(KERNEL) $(KERNEL_ENTRY_OBJ) $(KERNEL_C_OBJ)

# Assemble kernel entry point
$(KERNEL_ENTRY_OBJ): $(KERNEL_ENTRY)
	@mkdir -p build
	@echo "Assembling kernel entry..."
	$(ASM) $(ASMFLAGS) $(KERNEL_ENTRY) -o $(KERNEL_ENTRY_OBJ)

# Compile kernel C files
build/%.o: kernel/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/drivers/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/mm/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/fs/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/lib/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Run in QEMU emulator
run: $(OS_IMAGE)
	@echo "Starting XAE OS in QEMU..."
	qemu-system-i386 -drive file=$(OS_IMAGE),format=raw,index=0,media=disk

# Run with debugging
debug: $(OS_IMAGE)
	@echo "Starting XAE OS in QEMU (debug mode)..."
	qemu-system-i386 -drive file=$(OS_IMAGE),format=raw,index=0,media=disk -s -S

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf build/

# Create build directory
$(shell mkdir -p build)

.PHONY: all run debug clean
