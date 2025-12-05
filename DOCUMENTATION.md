# HOW IT ALL WORKS - A Deep Dive into XAE OS

This guide explains how each component of your OS works and why it's necessary.

---

## 🚀 THE BOOT PROCESS

### 1. Power On → BIOS
**WHAT HAPPENS:**
- You press the power button
- CPU starts executing BIOS code from ROM
- BIOS performs POST (Power-On Self Test)
- BIOS looks for bootable devices

**WHY:** The CPU needs initial instructions. BIOS provides this.

### 2. BIOS → Bootloader
**WHAT HAPPENS:**
- BIOS reads sector 1 from your disk (512 bytes)
- Checks for signature `0xAA55` at bytes 511-512
- If found, loads sector to memory address `0x7C00`
- Jumps to `0x7C00` to start executing your bootloader

**WHY:** BIOS doesn't know about filesystems. It just loads the first sector.

**YOUR CODE:** `boot/boot.asm`
```assembly
[ORG 0x7C00]    ; We're loaded at this address
start:
    ; Set up segments, print message, load kernel
```

### 3. Bootloader → Kernel
**WHAT HAPPENS:**
- Your bootloader uses BIOS interrupts to read more sectors
- Loads kernel to memory at `0x10000` (64KB mark)
- Switches CPU from 16-bit real mode to 32-bit protected mode
- Jumps to kernel entry point

**WHY:** 
- 16-bit mode can only access 1MB of RAM
- No memory protection in 16-bit mode
- 32-bit mode is needed for modern features

**YOUR CODE:** `kernel/entry.asm`
```assembly
; Switch to protected mode
mov eax, cr0
or eax, 1
mov cr0, eax
```

### 4. Kernel Initialization
**WHAT HAPPENS:**
- C code starts executing (`kernel_main`)
- Initialize VGA display
- Set up memory management
- Initialize filesystem
- Enter main loop

**WHY:** These are the core services every OS needs.

---

## 🖥️ VGA TEXT MODE DRIVER

### How it Works
VGA text mode is **memory-mapped I/O**:
- Physical address `0xB8000` is mapped to the screen
- Writing to this memory appears on screen instantly
- No need for complicated graphics calls!

### Memory Layout
```
Address     What
0xB8000     Character at row 0, column 0
0xB8001     Color for that character
0xB8002     Character at row 0, column 1
0xB8003     Color for that character
...
0xB8FA0     End of screen (80×25 = 2000 chars × 2 bytes)
```

### Color Byte Format
```
Bit:  7   6 5 4   3 2 1 0
      B   BG BG BG   FG FG FG FG
      |   |__________|  |_________|
      |        |             |
   Blink   Background   Foreground
```

**YOUR CODE:** `kernel/drivers/vga.c`
```c
static inline uint16_t vga_make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}
```

---

## 💾 MEMORY MANAGEMENT

### The Problem
- Your computer has RAM (let's say 32MB)
- How do you track which parts are free vs. used?
- How do you allocate memory to programs?

### The Solution: Page-Based Allocation
We divide memory into **pages** (4KB each):
- 32MB ÷ 4KB = 8,192 pages
- Use a **bitmap** to track them (1 bit per page)
- 8,192 bits = 1,024 bytes of overhead

### Bitmap Example
```
Bit:     0 1 2 3 4 5 6 7 ...
Value:   1 1 1 1 0 0 1 0 ...
Meaning: U U U U F F U F ...
         (U = Used, F = Free)
```

### Allocation Algorithm
```c
void* alloc_page(void) {
    for (page_num = 0; page_num < NUM_PAGES; page_num++) {
        if (!is_page_used(page_num)) {
            mark_as_used(page_num);
            return page_num * PAGE_SIZE;  // Return address
        }
    }
    return NULL;  // Out of memory!
}
```

**YOUR CODE:** `kernel/mm/memory.c`

---

## 📁 XAE FILESYSTEM (XAE-FS)

### Filesystem Basics
A filesystem is a way to organize data on storage. It handles:
1. **Where** is the data? (which blocks on disk)
2. **What** is the data? (filename, size, type)
3. **How** to access it? (read, write, delete)

### Traditional Filesystems (for comparison)
- **FAT32:** Uses a File Allocation Table (like a linked list)
- **ext4:** Uses inodes and extents
- **NTFS:** Uses Master File Table (MFT)

### YOUR Filesystem: XAE-FS

#### Disk Layout
```
Block 0:      Superblock (filesystem metadata)
Blocks 1-N:   Inode Table (file/directory info)
Blocks N+1-M: Data Blocks (actual file contents)
```

#### Superblock Structure
```c
struct xaefs_superblock {
    uint32_t magic;        // 0x58414546 ("XAEF")
    uint32_t version;      // FS version
    uint32_t block_size;   // 4096 bytes
    uint32_t total_blocks; // How many blocks total
    uint32_t free_blocks;  // How many available
    char label[32];        // Volume name
};
```

#### Inode Structure (File Metadata)
```c
struct xaefs_inode {
    char name[64];         // Filename
    uint32_t size;         // File size in bytes
    uint32_t block_start;  // First data block
    uint8_t type;          // File, directory, etc.
    
    // UNIQUE FEATURES:
    uint8_t priority;      // LOW, NORMAL, HIGH, CRITICAL
    uint16_t version;      // Version number
    char tags[8][16];      // Up to 8 tags per file
};
```

### Why Your FS is Unique

**1. Priority System**
```c
// High-priority files could be:
// - Backed up first
// - Loaded into cache
// - Protected from deletion
xaefs_set_priority("/important.txt", XAEFS_PRIORITY_CRITICAL);
```

**2. Tagging System**
```c
// Tag-based organization (like Gmail labels)
xaefs_add_tag("/report.doc", "work");
xaefs_add_tag("/report.doc", "Q4");
xaefs_add_tag("/report.doc", "important");
// Later: search by tag across all directories
```

**3. Built-in Versioning**
```c
// Filesystem tracks file versions automatically
xaefs_create_version("/config.sys");  // Save current state
// If something breaks, rollback to previous version
```

**YOUR CODE:** `kernel/fs/xaefs.c`

---

## 🔧 BUILD PROCESS

### Step-by-Step Compilation

**1. Assemble Bootloader**
```bash
nasm -f bin boot/boot.asm -o build/boot.bin
```
- Input: Assembly source code
- Output: Raw binary (512 bytes)
- No OS code, just pure machine instructions

**2. Assemble Kernel Entry**
```bash
nasm -f elf32 kernel/entry.asm -o build/entry.o
```
- Input: Assembly source
- Output: ELF object file (relocatable)
- Will be linked with C code

**3. Compile C Code**
```bash
gcc -m32 -ffreestanding -nostdlib -c kernel/kernel.c -o build/kernel.o
gcc -m32 -ffreestanding -nostdlib -c kernel/drivers/vga.c -o build/vga.o
# ... etc for all .c files
```
- `-m32`: 32-bit code
- `-ffreestanding`: No standard library
- `-nostdlib`: Don't link libc
- Each `.c` file becomes a `.o` object file

**4. Link Everything**
```bash
ld -m elf_i386 -T linker.ld -o build/kernel.bin entry.o kernel.o vga.o ...
```
- Takes all `.o` files
- Uses `linker.ld` script to organize memory
- Produces final kernel binary

**5. Create OS Image**
```bash
cat boot.bin kernel.bin > xae_os.img
```
- Concatenate bootloader + kernel
- This is your complete OS!

### The Linker Script
```ld
SECTIONS {
    . = 0x10000;      /* Start at 64KB */
    .text : { *(.text) }     /* Code */
    .data : { *(.data) }     /* Data */
    .bss  : { *(.bss) }      /* Uninitialized */
}
```
This tells the linker **where in memory** to put each section.

---

## 🎯 NEXT STEPS

### Features to Add

**1. Keyboard Driver**
- Read keyboard input
- Implement command shell
- Add basic commands (ls, cat, mkdir, etc.)

**2. Disk I/O**
- ATA/IDE disk driver
- Read/write filesystem to actual disk
- Persistence between reboots

**3. Interrupt Handling**
- Set up IDT (Interrupt Descriptor Table)
- Handle hardware interrupts (keyboard, timer)
- System calls

**4. Process Management**
- Task switching
- Scheduler
- User mode vs kernel mode

**5. Enhanced Filesystem**
- Implement actual disk writes
- Directory tree traversal
- File permissions

### Learning Resources
- [OSDev Wiki](https://wiki.osdev.org/) - Best OS development resource
- Intel Software Developer Manuals - CPU documentation
- "Operating Systems: Three Easy Pieces" - Free textbook

---

## 🐛 TROUBLESHOOTING

### "Cannot find QEMU"
Install QEMU emulator:
- Windows: Download from qemu.org
- Linux: `sudo apt install qemu-system-x86`
- macOS: `brew install qemu`

### "gcc: command not found"
Install build tools:
- Windows: Install MinGW or WSL
- Linux: `sudo apt install build-essential nasm`
- macOS: `xcode-select --install`

### "Bootloader doesn't run"
- Check boot signature (0xAA55 at end)
- Verify image size (must be at least 512 bytes)
- Try: `qemu-system-i386 -fda xae_os.img`

### "Kernel crashes immediately"
- Check linker script addresses
- Verify GDT is set up correctly
- Use QEMU debugger: `make debug` then `gdb`

---

## 📚 KEY CONCEPTS EXPLAINED

### Real Mode vs Protected Mode
- **Real Mode:** 16-bit, can access only 1MB RAM, no memory protection
- **Protected Mode:** 32-bit, can access 4GB RAM, memory protection, virtual memory

### Memory Segmentation
In real mode, addresses are calculated as:
```
Physical Address = (Segment × 16) + Offset
Example: 0x7C00 = (0x07C0 × 16) + 0x0000
```

### Why We Can't Use libc
Standard C library assumes:
- Operating system exists
- System calls available (malloc, printf, file I/O)
- Runtime environment set up

In kernel mode, **we ARE the operating system**, so we write our own!

### Boot Signature Magic Number
`0xAA55` is chosen because:
- Easy to recognize (0x55 = 01010101, 0xAA = 10101010)
- Unlikely to occur by chance
- Legacy from original IBM PC BIOS

---

**Congratulations!** You now understand how a complete operating system works from the first CPU instruction to filesystem operations. This is the foundation for building more advanced features!
