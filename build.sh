# Easy Build & Run Script for WSL/Linux
# Make this executable: chmod +x build.sh

#!/bin/bash

echo "XAE OS Build System"
echo "==================="
echo ""

# Check for tools
for tool in nasm gcc ld; do
    if ! command -v $tool &> /dev/null; then
        echo "ERROR: $tool not found"
        echo "Install with: sudo apt install build-essential nasm"
        exit 1
    fi
done

# Create build directory
mkdir -p build

# Step 1: Bootloader
echo "Step 1: Assembling bootloader..."
nasm -f bin boot/boot.asm -o build/boot.bin || exit 1

# Step 2: Kernel entry
echo "Step 2: Assembling kernel entry..."
nasm -f elf32 kernel/entry.asm -o build/entry.o || exit 1

# Step 3: Compile C files
echo "Step 3: Compiling C sources..."
CFLAGS="-m32 -ffreestanding -nostdlib -fno-pie -Wall -Wextra -O2 -I kernel"

gcc $CFLAGS -c kernel/kernel.c -o build/kernel.o || exit 1
gcc $CFLAGS -c kernel/drivers/vga.c -o build/vga.o || exit 1
gcc $CFLAGS -c kernel/mm/memory.c -o build/memory.o || exit 1
gcc $CFLAGS -c kernel/fs/xaefs.c -o build/xaefs.o || exit 1
gcc $CFLAGS -c kernel/lib/string.c -o build/string.o || exit 1

# Step 4: Link
echo "Step 4: Linking kernel..."
ld -m elf_i386 -T linker.ld -o build/kernel.bin \
    build/entry.o build/kernel.o build/vga.o build/memory.o \
    build/xaefs.o build/string.o || exit 1

# Step 5: Create OS image
echo "Step 5: Creating OS image..."
cat build/boot.bin build/kernel.bin > build/xae_os.img

echo ""
echo "SUCCESS! OS built successfully!"
echo "Image: build/xae_os.img"
echo ""

# Ask if user wants to run
read -p "Run in QEMU? (y/n) " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    if command -v qemu-system-i386 &> /dev/null; then
        qemu-system-i386 -drive file=build/xae_os.img,format=raw,index=0,media=disk
    else
        echo "QEMU not installed. Install with: sudo apt install qemu-system-x86"
    fi
fi
