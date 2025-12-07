/*
 * ==============================================================================
 * KEYBOARD DRIVER IMPLEMENTATION
 * ==============================================================================
 */

#include "include/keyboard.h"
#include "include/vga.h"

/* US QWERTY keyboard layout - scan code to ASCII */
static const char scancode_map[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};

/* Shifted characters */
static const char scancode_shift_map[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0, ' '
};

static uint8_t shift_pressed = 0;

/* Read byte from I/O port */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * keyboard_init() - Initialize keyboard driver
 */
void keyboard_init(void) 
{
    /* Keyboard is ready - PS/2 controller is already initialized by BIOS */
    shift_pressed = 0;
}

/*
 * keyboard_has_input() - Check if keyboard has data available
 * 
 * WHAT: Non-blocking check for keyboard input
 * WHY: Allows polling without blocking
 * RETURNS: 1 if key available, 0 otherwise
 */
uint8_t keyboard_has_input(void)
{
    /* Check if data is available (bit 0 of status register) */
    return (inb(0x64) & 0x01) != 0;
}

/*
 * keyboard_getchar() - Get one character from keyboard
 * 
 * WHAT: Wait for keypress and return ASCII character
 * WHY: Basic input function for reading user commands
 * HOW: Poll keyboard controller until key available, convert scan code
 * RETURNS: ASCII character
 */
char keyboard_getchar(void) 
{
    uint8_t scancode;
    
    /* Wait for key press */
    while (1) {
        /* Check if data is available (bit 0 of status register) */
        if (inb(0x64) & 0x01) {
            scancode = inb(0x60);
            
            /* Check for shift press/release */
            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = 1;
                continue;
            }
            if (scancode == 0xAA || scancode == 0xB6) {
                shift_pressed = 0;
                continue;
            }
            
            /* Ignore key release (high bit set) */
            if (scancode & 0x80) {
                continue;
            }
            
            /* Convert scan code to ASCII */
            if (scancode < sizeof(scancode_map)) {
                if (shift_pressed) {
                    return scancode_shift_map[scancode];
                } else {
                    return scancode_map[scancode];
                }
            }
        }
    }
}

/*
 * keyboard_readline() - Read a line of input
 * 
 * WHAT: Read characters until Enter is pressed
 * WHY: To read command input from user
 * HOW: Echo characters, handle backspace, stop at newline
 */
void keyboard_readline(char* buffer, uint32_t max_len) 
{
    uint32_t i = 0;
    char c;
    
    while (1) {
        c = keyboard_getchar();
        
        if (c == '\n') {
            /* Enter pressed - finish input */
            vga_putchar('\n');
            buffer[i] = '\0';
            return;
        }
        else if (c == '\b') {
            /* Backspace - remove last character */
            if (i > 0) {
                i--;
                /* Move cursor back, write space, move back again */
                vga_putchar('\b');
                vga_putchar(' ');
                vga_putchar('\b');
            }
        }
        else if (c >= 32 && c <= 126 && i < max_len - 1) {
            /* Printable character - add to buffer and echo */
            buffer[i++] = c;
            vga_putchar(c);
        }
        /* Ignore all other characters (non-printable, out of range, etc.) */
    }
}
