/*
 * ==============================================================================
 * VGA TEXT MODE DRIVER
 * ==============================================================================
 * WHAT: Driver to display text on screen
 * WHY: Without this, we can't show any output to the user
 * HOW: VGA text mode uses memory-mapped I/O at address 0xB8000
 * 
 * TECHNICAL DETAILS:
 * - VGA text mode has 80 columns x 25 rows = 2000 characters
 * - Each character takes 2 bytes:
 *   Byte 0: ASCII character code
 *   Byte 1: Attribute (color: 4 bits background, 4 bits foreground)
 * - Memory address: 0xB8000 - 0xB8FA0
 */

#ifndef VGA_H
#define VGA_H

#include <stdint.h>

/* VGA Constants */
#define VGA_WIDTH 80                    /* Characters per row */
#define VGA_HEIGHT 25                   /* Rows per screen */
#define VGA_MEMORY 0xB8000              /* Physical address of VGA buffer */

/* VGA Colors 
 * WHY: These are the 16 colors available in VGA text mode
 * HOW: Combine background and foreground to create color attributes */
enum vga_color {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW = 14,
    VGA_WHITE = 15,
};

/* Function declarations */
void vga_init(void);
void vga_clear(void);
void vga_putchar(char c);
void vga_print(const char* str);
void vga_set_color(uint8_t fg, uint8_t bg);

#endif /* VGA_H */
