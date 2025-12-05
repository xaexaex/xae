/*
 * ==============================================================================
 * VGA DRIVER IMPLEMENTATION
 * ==============================================================================
 */

#include "include/vga.h"

/* Global state variables */
static uint16_t* vga_buffer;    /* Pointer to VGA memory */
static uint32_t vga_index;      /* Current cursor position (0-1999) */
static uint8_t vga_color;       /* Current color attribute */

/* I/O port functions for cursor control */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * vga_make_color() - Create a color attribute byte
 * 
 * WHAT: Combines foreground and background colors into one byte
 * WHY: VGA needs colors packed as: [4-bit bg][4-bit fg]
 * HOW: Shift background left 4 bits, OR with foreground
 */
static inline uint8_t vga_make_color(uint8_t fg, uint8_t bg) 
{
    return fg | (bg << 4);
}

/*
 * vga_make_entry() - Create a full VGA character entry
 * 
 * WHAT: Combines ASCII character and color into 16-bit VGA entry
 * WHY: VGA memory stores character in low byte, color in high byte
 * HOW: [8-bit color][8-bit character]
 */
static inline uint16_t vga_make_entry(char c, uint8_t color) 
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

/*
 * vga_update_cursor() - Update hardware cursor position
 * 
 * WHAT: Move the blinking cursor to current position
 * WHY: So users can see where they're typing
 * HOW: Write position to VGA controller ports
 */
void vga_update_cursor(void) 
{
    uint16_t pos = vga_index;
    
    /* Cursor position: low byte */
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    
    /* Cursor position: high byte */
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/*
 * vga_init() - Initialize VGA driver
 * 
 * WHAT: Set up the VGA driver for text output
 * WHY: Must be called before any other VGA functions
 * HOW: Set pointer to VGA memory, choose default colors
 */
void vga_init(void) 
{
    vga_buffer = (uint16_t*)VGA_MEMORY;
    vga_index = 0;
    vga_color = vga_make_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_update_cursor();
}

/*
 * vga_clear() - Clear the entire screen
 * 
 * WHAT: Fill screen with spaces
 * WHY: To start fresh or clear output
 * HOW: Write space character to all 2000 positions
 */
void vga_clear(void) 
{
    uint32_t i;
    uint16_t blank = vga_make_entry(' ', vga_color);
    
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    
    vga_index = 0;
    vga_update_cursor();
}

/*
 * vga_scroll() - Scroll screen up one line
 * 
 * WHAT: Move all lines up, clear bottom line
 * WHY: When we reach bottom of screen, we need to scroll
 * HOW: Copy each line to the line above it, clear last line
 */
static void vga_scroll(void) 
{
    uint32_t i;
    uint16_t blank = vga_make_entry(' ', vga_color);
    
    /* Copy each line to the previous line */
    for (i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    
    /* Clear the last line */
    for (i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
        vga_buffer[i] = blank;
    }
    
    /* Move cursor to start of last line */
    vga_index = (VGA_HEIGHT - 1) * VGA_WIDTH;
    vga_update_cursor();
}

/*
 * vga_putchar() - Display a single character
 * 
 * WHAT: Put one character on screen at cursor position
 * WHY: Basic building block for all text output
 * HOW: Handle special characters (newline, tab), write to VGA memory
 */
void vga_putchar(char c) 
{
    /* Handle special characters */
    if (c == '\n') {
        /* Newline: move to start of next line */
        vga_index = (vga_index / VGA_WIDTH + 1) * VGA_WIDTH;
    } 
    else if (c == '\r') {
        /* Carriage return: move to start of current line */
        vga_index = (vga_index / VGA_WIDTH) * VGA_WIDTH;
    }
    else if (c == '\b') {
        /* Backspace: move back one position */
        if (vga_index > 0) {
            vga_index--;
        }
    }
    else if (c == '\t') {
        /* Tab: move to next 4-space boundary */
        vga_index = (vga_index + 4) & ~(4 - 1);
    }
    else if (c >= 32 && c <= 126) {
        /* Regular printable character: write to VGA memory */
        vga_buffer[vga_index] = vga_make_entry(c, vga_color);
        vga_index++;
    }
    /* Ignore non-printable characters */
    
    /* If we've gone past the screen, scroll up */
    if (vga_index >= VGA_WIDTH * VGA_HEIGHT) {
        vga_scroll();
    }
    
    vga_update_cursor();
}

/*
 * vga_print() - Print a string to screen
 * 
 * WHAT: Display a null-terminated string
 * WHY: More convenient than calling putchar repeatedly
 * HOW: Loop through each character until we hit '\0'
 */
void vga_print(const char* str) 
{
    uint32_t i = 0;
    
    while (str[i] != '\0') {
        vga_putchar(str[i]);
        i++;
    }
}

/*
 * vga_set_color() - Change text color
 * 
 * WHAT: Set foreground and background colors for future text
 * WHY: To highlight important messages or create visual structure
 * HOW: Update the color attribute byte
 */
void vga_set_color(uint8_t fg, uint8_t bg) 
{
    vga_color = vga_make_color(fg, bg);
}
