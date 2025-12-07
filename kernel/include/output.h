/*
 * ==============================================================================
 * UNIFIED OUTPUT SYSTEM
 * ==============================================================================
 * WHAT: Single print function that outputs to both VGA and serial
 * WHY: So remote users see the same output as local users
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include "vga.h"
#include "serial.h"

/* Print to both VGA and serial */
static inline void print(const char* str) {
    vga_print(str);
    serial_print(str);
}

/* Print character to both outputs */
static inline void putchar_dual(char c) {
    vga_putchar(c);
    serial_putchar(c);
}

#endif /* OUTPUT_H */
