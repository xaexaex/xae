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
KERNEL_C_SRC = $(wildcard kernel/*.c kernel/drivers/*.c kernel/mm/*.c kernel/fs/*.c kernel/lib/*.c kernel/shell/*.c kernel/editor/*.c kernel/net/*.c kernel/auth/*.c)

# Object files
KERNEL_ENTRY_OBJ = build/entry.o
KERNEL_C_OBJ = $(patsubst %.c, build/%.o, $(notdir $(KERNEL_C_SRC)))

# Output files
BOOTLOADER = build/boot.bin
KERNEL = build/kernel.bin
OS_IMAGE = build/xae_os.img
DISK_IMAGE = build/xae_disk.img

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

build/%.o: kernel/shell/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/editor/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/net/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: kernel/auth/%.c
	@mkdir -p build
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Run in QEMU emulator (local VGA mode)
run: $(OS_IMAGE) $(DISK_IMAGE)
	@echo "Starting XAE OS in QEMU (local mode)..."
	qemu-system-i386 \
		-drive file=$(OS_IMAGE),format=raw,if=floppy \
		-drive file=$(DISK_IMAGE),format=raw,if=ide,index=1 \
		-boot a

# Run with network support (RTL8139 NIC)
runnet: $(OS_IMAGE) $(DISK_IMAGE)
	@echo "=========================================="
	@echo "XAE OS - Network Edition"
	@echo "=========================================="
	@echo "Network: 10.0.0.2 (XAE OS)"
	@echo "         10.0.0.1 (Gateway)"
	@echo ""
	@echo "Connect using Python client:"
	@echo "  python3 xae_client.py 127.0.0.1 2323 admin admin123"
	@echo ""
	@echo "Default users:"
	@echo "  admin / admin123"
	@echo "  user  / password"
	@echo "=========================================="
	qemu-system-i386 \
		-drive file=$(OS_IMAGE),format=raw,if=floppy \
		-drive file=$(DISK_IMAGE),format=raw,if=ide,index=1 \
		-boot a \
		-netdev user,id=net0,hostfwd=tcp::2323-10.0.0.2:23 \
		-device rtl8139,netdev=net0 \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# Run with serial port for remote access
runserial: $(OS_IMAGE) $(DISK_IMAGE)
	@echo "=========================================="
	@echo "XAE OS - Remote Access Mode"
	@echo "=========================================="
	@echo "Serial port redirected to TCP port 4444"
	@echo ""
	@echo "Connect from another terminal:"
	@echo "  telnet localhost 4444"
	@echo "  OR: nc localhost 4444"
	@echo "=========================================="
	qemu-system-i386 -drive file=$(OS_IMAGE),format=raw,index=0,media=disk -drive file=$(DISK_IMAGE),format=raw,index=1,media=disk -serial tcp::4444,server,nowait -nographic

# Create persistent disk image
$(DISK_IMAGE):
	@echo "Creating persistent disk image (10MB)..."
	@dd if=/dev/zero of=$(DISK_IMAGE) bs=1M count=10 2>/dev/null || (echo "Failed to create disk image" && exit 1)
	@echo "Disk image created successfully"

# Run with debugging
debug: $(OS_IMAGE) $(DISK_IMAGE)
	@echo "Starting XAE OS in QEMU (debug mode)..."
	qemu-system-i386 -drive file=$(OS_IMAGE),format=raw,index=0,media=disk -drive file=$(DISK_IMAGE),format=raw,index=1,media=disk -s -S

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf build/

# Create build directory
$(shell mkdir -p build)

.PHONY: all run debug clean
