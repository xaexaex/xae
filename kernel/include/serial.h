/*
 * ==============================================================================
 * SERIAL PORT DRIVER (COM1)
 * ==============================================================================
 * WHAT: Serial port communication for remote terminal access
 * WHY: Allows users to connect via telnet/minicom instead of QEMU window
 * HOW: Uses COM1 (I/O port 0x3F8) for communication
 */

#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

/* Serial port I/O addresses */
#define COM1_PORT 0x3F8

/* Initialize serial port */
void serial_init(void);

/* Check if serial port can send data */
int serial_can_write(void);

/* Check if serial port has received data */
int serial_can_read(void);

/* Drain any pending RX bytes */
void serial_flush_input(void);

/* Write a character to serial port */
void serial_putchar(char c);

/* Read a character from serial port */
char serial_getchar(void);

/* Write a string to serial port */
void serial_print(const char* str);

/* Read a line from serial port */
void serial_readline(char* buffer, uint32_t max_len);

#endif /* SERIAL_H */
