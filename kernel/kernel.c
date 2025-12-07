/*
 * ==============================================================================
 * XAE KERNEL - Main Entry Point
 * ==============================================================================
 * WHAT: This is the heart of your operating system
 * WHY: After the bootloader loads us into memory, it jumps here
 * HOW: We initialize hardware, set up memory management, and create our filesystem
 */

#include "include/vga.h"
#include "include/memory.h"
#include "include/xaefs.h"
#include "include/keyboard.h"
#include "include/shell.h"
#include "include/disk.h"
#include "include/serial.h"
#include "include/rtl8139.h"
#include "include/net.h"
#include "include/auth.h"

/*
 * kernel_main() - The first C function that runs
 * 
 * WHAT: This is where your OS starts after the bootloader
 * WHY: The bootloader is written in assembly and is limited. Here we can use C!
 * HOW: We'll initialize each subsystem one by one
 */
void kernel_main(void) 
{
    /* Initialize display and serial */
    vga_init();
    vga_clear();
    vga_print("KERNEL STARTED!\n");
    
    serial_init();
    
    /* Welcome messages */
    vga_clear();
    vga_print("XAE OS v0.2 - Network Edition\n");
    vga_print("==============================\n");
    
    /* Initialize subsystems */
    memory_init();
    vga_print("Memory initialized\n");
    
    disk_init();
    vga_print("Disk initialized\n");
    
    /* Initialize networking */
    vga_print("\n--- Network Initialization ---\n");
    rtl8139_init();
    net_init();
    auth_init();
    vga_print("--- Network Ready ---\n\n");
    
    vga_print("Listening for connections on port 23\n");
    vga_print("Default users: admin/admin123, user/password\n\n");
    
    /* Load or create filesystem */
    xaefs_load();
    if (!xaefs_is_loaded()) {
        xaefs_init();
        xaefs_format("XAE_FS_DISK");
        xaefs_sync();
    }
    
    serial_print("[OK] Ready\r\n");
    serial_print("Type 'help' for commands\r\n\r\n");
    
    /* Initialize keyboard and shell */
    keyboard_init();
    shell_init();
    
    /* Run the shell - it will handle both keyboard and network */
    shell_run();
    
    /* Should never reach here */
    while(1) {
        __asm__ __volatile__("hlt");
    }
}
