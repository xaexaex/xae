# Quick Start Guide

## Prerequisites

### Windows
1. Install WSL (Windows Subsystem for Linux):
   ```powershell
   wsl --install
   ```

2. Inside WSL, install tools:
   ```bash
   sudo apt update
   sudo apt install build-essential nasm qemu-system-x86
   ```

### Linux
```bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86
```

### macOS
```bash
# Install Homebrew if you haven't
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install tools
brew install nasm qemu i686-elf-gcc
```

## Building Your OS

1. Open terminal in the project directory
2. Run:
   ```bash
   make
   ```

This will:
- Assemble the bootloader
- Compile all C code
- Link everything together
- Create `build/xae_os.img`

## Running Your OS

```bash
make run
```

This launches your OS in QEMU emulator. You should see:
```
XAE OS Booting...
Loading kernel...
XAE Operating System v0.1
========================

[OK] VGA Display initialized
[..] Initializing memory manager...
[OK] Memory manager initialized
...
```

## What You'll See

Your OS will:
1. Boot and display the XAE logo
2. Initialize all subsystems
3. Create demo files in the filesystem
4. List all files with their priorities and tags
5. Show the unique features of XAE-FS

## Basic Commands (Makefile)

```bash
make        # Build the OS
make run    # Run in QEMU
make clean  # Remove build files
make debug  # Run with GDB debugging
```

## Testing Your Filesystem

The kernel automatically creates demo files:
- `readme.txt` - Normal priority, tagged "docs"
- `kernel.c` - High priority, tagged "source" and "critical"
- `bin/` - Directory with high priority
- `config.sys` - Critical priority, tagged "system"

You can see these listed when the OS boots!

## Next Steps

1. Read `DOCUMENTATION.md` for detailed explanations
2. Modify `kernel/kernel.c` to add your own demo files
3. Try adding new features to the filesystem
4. Implement keyboard input (see DOCUMENTATION.md)

## Troubleshooting

**Black screen?**
- Check bootloader compiled correctly
- Verify boot signature: `hexdump -C build/boot.bin | tail`
  (Should end with `55 aa`)

**Kernel doesn't load?**
- Check linker.ld addresses
- Verify kernel.bin exists in build/

**QEMU not found?**
- Make sure QEMU is installed
- On Windows, use WSL or add QEMU to PATH

## Understanding the Code

Every file has extensive comments explaining:
- **WHAT** the code does
- **WHY** it's necessary  
- **HOW** it works

Start reading from:
1. `boot/boot.asm` - See how the first code runs
2. `kernel/entry.asm` - Understand the mode switch
3. `kernel/kernel.c` - Follow the initialization
4. `kernel/fs/xaefs.c` - Explore your filesystem

Have fun building your OS! 🚀
