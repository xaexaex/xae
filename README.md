# XAE Operating System Kernel

A custom operating system kernel built from scratch in C, featuring a unique filesystem design.

## What You're Building

This OS kernel teaches you the fundamentals of how operating systems work:

1. **Bootloader** - The first code that runs when your computer starts
2. **Kernel** - The core of the OS that manages hardware and resources
3. **Memory Management** - How the OS tracks and allocates RAM
4. **Filesystem** - Your own unique way of organizing files on disk
5. **Device Drivers** - Code to communicate with hardware (keyboard, screen, etc.)

## Project Structure

```
/boot          - Bootloader code (starts your OS)
/kernel        - Core kernel code
  /drivers     - Hardware drivers (VGA, keyboard, etc.)
  /mm          - Memory management
  /fs          - Your custom filesystem
  /include     - Header files
/build         - Compiled output files
```

## Building & Running

```bash
make           # Compile everything
make run       # Run in QEMU emulator
make clean     # Clean build files
```

## Learning Path

Each component is documented with inline comments explaining:
- WHAT the code does
- WHY it's necessary
- HOW it works at a low level
