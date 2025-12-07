/*
 * ==============================================================================
 * KEYBOARD DRIVER
 * ==============================================================================
 * WHAT: Reads keyboard input from PS/2 keyboard controller
 * WHY: To allow user interaction with the OS
 * HOW: Read scan codes from port 0x60, convert to ASCII
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* Initialize keyboard driver */
void keyboard_init(void);

/* Check if keyboard has input available (non-blocking) */
uint8_t keyboard_has_input(void);

/* Get a character from keyboard (blocking) */
char keyboard_getchar(void);

/* Read a line of input into buffer */
void keyboard_readline(char* buffer, uint32_t max_len);

#endif /* KEYBOARD_H */
