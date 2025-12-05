/*
 * ==============================================================================
 * XAE KERNEL - Main Entry Point
 * ==============================================================================
 * WHAT: This is the heart of your operating system
 * WHY: After the bootloader loads us into memory, it jumps here
 * HOW: We initialize hardware, set up memory management, and create our filesystem
 */

#include "include/vga.h"
#include "include/string.h"
#include "include/memory.h"
#include "include/xaefs.h"

/*
 * kernel_main() - The first C function that runs
 * 
 * WHAT: This is where your OS starts after the bootloader
 * WHY: The bootloader is written in assembly and is limited. Here we can use C!
 * HOW: We'll initialize each subsystem one by one
 */
void kernel_main(void) 
{
    /* STEP 1: Initialize the display
     * WHY: So we can show messages to the user */
    vga_init();
    
    /* Clear screen and show welcome message */
    vga_clear();
    vga_print("XAE Operating System v0.1\n");
    vga_print("========================\n\n");
    
    vga_print("[OK] VGA Display initialized\n");
    
    /* STEP 2: Initialize memory management
     * WHY: We need to track which parts of RAM are free/used
     * HOW: We'll create a bitmap to track 4KB pages of memory */
    vga_print("[..] Initializing memory manager...\n");
    memory_init();
    vga_print("[OK] Memory manager initialized\n");
    
    /* STEP 3: Show some system info */
    vga_print("\nSystem Information:\n");
    vga_print("  - Architecture: x86 (32-bit)\n");
    vga_print("  - Available Memory: Detecting...\n");
    vga_print("  - Filesystem: XAE-FS (custom)\n");
    
    /* STEP 4: Initialize our custom filesystem
     * WHY: We need a way to organize files on disk
     * HOW: We'll create our own unique filesystem structure */
    vga_print("\n[..] Initializing XAE Filesystem...\n");
    xaefs_init();
    xaefs_format("XAE System Disk");
    vga_print("[OK] Filesystem ready\n");
    
    vga_print("\n");
    vga_print("Kernel initialized successfully!\n");
    vga_print("\n=== FILESYSTEM DEMONSTRATION ===\n");
    
    /* Create some demo files to show off our filesystem! */
    vga_print("\nCreating demo files...\n");
    
    xaefs_create("readme.txt", XAEFS_FILE_REGULAR, XAEFS_PRIORITY_NORMAL);
    xaefs_add_tag("readme.txt", "docs");
    
    xaefs_create("kernel.c", XAEFS_FILE_REGULAR, XAEFS_PRIORITY_HIGH);
    xaefs_add_tag("kernel.c", "source");
    xaefs_add_tag("kernel.c", "critical");
    
    xaefs_mkdir("bin", XAEFS_PRIORITY_HIGH);
    xaefs_add_tag("bin", "executables");
    
    xaefs_create("config.sys", XAEFS_FILE_REGULAR, XAEFS_PRIORITY_CRITICAL);
    xaefs_add_tag("config.sys", "system");
    
    /* List all files */
    xaefs_list_dir("/");
    
    vga_print("\n=== UNIQUE FEATURES DEMO ===\n");
    vga_print("1. File Priorities: Each file has a priority level\n");
    vga_print("2. Tagging System: Files can have multiple tags\n");
    vga_print("3. Version Control: Built-in versioning support\n");
    
    vga_print("\nReady to accept commands.\n\n");
    vga_print("> ");
    
    /* STEP 5: Main kernel loop
     * WHY: The kernel needs to keep running and respond to events
     * HOW: For now, we just loop forever. Later we'll add keyboard input */
    while(1) {
        // TODO: Add keyboard driver and command processing
        // For now, just halt to save CPU
        __asm__ __volatile__("hlt");
    }
}
