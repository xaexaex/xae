/*
 * ==============================================================================
 * LINKER SCRIPT
 * ==============================================================================
 * WHAT: Tells the linker how to organize our kernel in memory
 * WHY: We need precise control over where code/data is loaded
 * HOW: Define memory sections and their addresses
 * 
 * MEMORY LAYOUT:
 * 0x00000000 - 0x000003FF : Real Mode IVT (BIOS)
 * 0x00000400 - 0x000004FF : BIOS Data Area
 * 0x00000500 - 0x00007BFF : Free memory
 * 0x00007C00 - 0x00007DFF : Bootloader (512 bytes)
 * 0x00007E00 - 0x0009FFFF : Free memory
 * 0x000A0000 - 0x000FFFFF : Video memory, ROM
 * 0x00010000 - ...        : Our kernel loads here!
 */

OUTPUT_FORMAT("binary")
ENTRY(kernel_entry)

SECTIONS
{
    /* Kernel loads at 1MB (0x10000) */
    . = 0x10000;

    /* Code section - executable code goes here */
    .text : ALIGN(4096)
    {
        *(.text)     /* All .text sections from all object files */
    }

    /* Read-only data - constants, strings, etc. */
    .rodata : ALIGN(4096)
    {
        *(.rodata)
    }

    /* Initialized data - global variables with initial values */
    .data : ALIGN(4096)
    {
        *(.data)
    }

    /* Uninitialized data - global variables without initial values */
    .bss : ALIGN(4096)
    {
        *(COMMON)
        *(.bss)
    }

    /* Discard unnecessary sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}
