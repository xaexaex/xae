/*
 * ==============================================================================
 * SERIAL PORT DRIVER IMPLEMENTATION
 * ==============================================================================
 */

#include "include/serial.h"

/* I/O port operations */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/*
 * serial_init() - Initialize serial port COM1
 */
void serial_init(void) 
{
    /* Disable interrupts */
    outb(COM1_PORT + 1, 0x00);
    
    /* Enable DLAB (set baud rate divisor) */
    outb(COM1_PORT + 3, 0x80);
    
    /* Set divisor to 3 (38400 baud) */
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    
    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 3, 0x03);
    
    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(COM1_PORT + 2, 0xC7);
    
    /* IRQs enabled, RTS/DSR set */
    outb(COM1_PORT + 4, 0x0B);
    
    /* Send telnet control codes to disable local echo */
    /* Wait for port to be ready, then send IAC WILL ECHO */
    while (!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, 255);  /* IAC */
    while (!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, 251);  /* WILL */
    while (!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, 1);    /* ECHO */
    
    /* IAC WILL SUPPRESS-GO-AHEAD */
    while (!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, 255);  /* IAC */
    while (!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, 251);  /* WILL */
    while (!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, 3);    /* SUPPRESS-GO-AHEAD */
    
    /* IAC DO SUPPRESS-GO-AHEAD */
    while (!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, 255);  /* IAC */
    while (!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, 253);  /* DO */
    while (!(inb(COM1_PORT + 5) & 0x20));
    outb(COM1_PORT, 3);    /* SUPPRESS-GO-AHEAD */
}

/*
 * serial_can_write() - Check if transmit buffer is empty
 */
int serial_can_write(void) 
{
    return inb(COM1_PORT + 5) & 0x20;
}

/*
 * serial_can_read() - Check if data is available
 */
int serial_can_read(void) 
{
    return inb(COM1_PORT + 5) & 0x01;
}

/*
 * serial_flush_input() - Drain any pending RX bytes
 * Use before prompting to discard stale input (e.g., keystrokes sent before prompts)
 */
void serial_flush_input(void)
{
    while (serial_can_read()) {
        (void)inb(COM1_PORT);
    }
}

/*
 * serial_putchar() - Write a character to serial port
 */
void serial_putchar(char c) 
{
    /* Wait until transmit buffer is empty */
    while (!serial_can_write());
    
    /* Send character */
    outb(COM1_PORT, c);
}

/*
 * serial_getchar() - Read a character from serial port
 */
char serial_getchar(void) 
{
    /* Wait until data is available */
    while (!serial_can_read());
    
    /* Read character */
    return inb(COM1_PORT);
}

/*
 * serial_print() - Write a string to serial port
 */
void serial_print(const char* str) 
{
    uint32_t i = 0;
    while (str[i] != '\0') {
        serial_putchar(str[i]);
        i++;
    }
}

/*
 * serial_readline() - Read a line from serial port
 */
void serial_readline(char* buffer, uint32_t max_len) 
{
    uint32_t i = 0;
    char c;
    
    while (i < max_len - 1) {
        c = serial_getchar();
        
        /* Handle backspace */
        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
            }
            continue;
        }
        
        /* Handle newline (consume optional CRLF pair) */
        if (c == '\r' || c == '\n') {
            if (c == '\r' && serial_can_read()) {
                char next = serial_getchar();
                if (next != '\n') {
                    /* Put back if it's not part of CRLF */
                    /* Simple fallback: ignore since buffering is minimal */
                }
            }
            break;
        }
        
        /* Regular character - telnet already echoes, don't echo again */
        if (c >= 32 && c <= 126) {
            buffer[i++] = c;
        }
    }
    
    buffer[i] = '\0';
}
