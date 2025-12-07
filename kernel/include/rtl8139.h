#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

// RTL8139 registers
#define RTL8139_IDR0      0x00  // MAC address
#define RTL8139_MAR0      0x08  // Multicast filter
#define RTL8139_TXSTATUS0 0x10  // Transmit status
#define RTL8139_TXADDR0   0x20  // Transmit start address
#define RTL8139_RXBUF     0x30  // Receive buffer start address
#define RTL8139_CMD       0x37  // Command register
#define RTL8139_RXBUFPTR  0x38  // Current read address
#define RTL8139_RXBUFADDR 0x3A  // Current buffer address
#define RTL8139_IMR       0x3C  // Interrupt mask
#define RTL8139_ISR       0x3E  // Interrupt status
#define RTL8139_TCR       0x40  // Transmit config
#define RTL8139_RCR       0x44  // Receive config
#define RTL8139_CONFIG1   0x52  // Configuration 1

// Command register bits
#define RTL8139_CMD_RESET  0x10
#define RTL8139_CMD_RX_EN  0x08
#define RTL8139_CMD_TX_EN  0x04

// Interrupt bits
#define RTL8139_INT_ROK    0x01  // Receive OK
#define RTL8139_INT_TOK    0x04  // Transmit OK

// Receive config
#define RTL8139_RCR_AAP    0x01  // Accept all packets
#define RTL8139_RCR_APM    0x02  // Accept physical match
#define RTL8139_RCR_AM     0x04  // Accept multicast
#define RTL8139_RCR_AB     0x08  // Accept broadcast
#define RTL8139_RCR_WRAP   0x80  // Wrap at end of buffer

// RTL8139 driver
void rtl8139_init(void);
void rtl8139_send_packet(const uint8_t* data, uint16_t length);
void rtl8139_handle_interrupt(void);
uint8_t rtl8139_get_mac(uint8_t index);

#endif
