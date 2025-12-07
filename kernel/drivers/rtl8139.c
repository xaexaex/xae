#include "include/rtl8139.h"
#include "include/vga.h"
#include <stdint.h>

// PCI configuration
#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

static uint32_t rtl8139_io_base = 0;
static uint8_t* rx_buffer;
static uint8_t* tx_buffers[4];
static uint8_t tx_current = 0;
static uint16_t rx_offset = 0;

// Receive buffer (8KB + 16 bytes for wraparound)
static uint8_t rx_buf[8192 + 16 + 1500] __attribute__((aligned(4)));
static uint8_t tx_buf[4][1536] __attribute__((aligned(4)));

// Port I/O helpers
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Find RTL8139 on PCI bus
static uint32_t pci_find_device(uint16_t vendor_id, uint16_t device_id) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            uint32_t address = 0x80000000 | (bus << 16) | (device << 11);
            outl(PCI_CONFIG_ADDRESS, address);
            uint32_t value = inl(PCI_CONFIG_DATA);
            
            if ((value & 0xFFFF) == vendor_id && ((value >> 16) & 0xFFFF) == device_id) {
                return address;
            }
        }
    }
    return 0;
}

void rtl8139_init(void) {
    vga_print("Searching for RTL8139 on PCI bus...\n");
    
    // Find RTL8139 on PCI bus (Vendor: 0x10EC, Device: 0x8139)
    uint32_t pci_addr = pci_find_device(0x10EC, 0x8139);
    if (pci_addr == 0) {
        vga_print("ERROR: RTL8139 not found!\n");
        vga_print("Make sure QEMU has: -device rtl8139,netdev=net0\n");
        return;
    }
    
    vga_print("RTL8139 found on PCI!\n");
    
    // Get I/O base address from PCI BAR0
    outl(PCI_CONFIG_ADDRESS, pci_addr | 0x10);
    rtl8139_io_base = inl(PCI_CONFIG_DATA) & 0xFFFFFFFC;
    
    vga_print("RTL8139 found at I/O base: 0x");
    vga_print_hex(rtl8139_io_base);
    vga_print("\n");
    
    // Enable PCI bus mastering
    outl(PCI_CONFIG_ADDRESS, pci_addr | 0x04);
    uint32_t cmd = inl(PCI_CONFIG_DATA);
    outl(PCI_CONFIG_ADDRESS, pci_addr | 0x04);
    outl(PCI_CONFIG_DATA, cmd | 0x05);
    
    // Power on the device
    outb(rtl8139_io_base + RTL8139_CONFIG1, 0x00);
    
    // Software reset
    outb(rtl8139_io_base + RTL8139_CMD, RTL8139_CMD_RESET);
    while ((inb(rtl8139_io_base + RTL8139_CMD) & RTL8139_CMD_RESET) != 0);
    
    // Initialize buffers
    rx_buffer = rx_buf;
    for (int i = 0; i < 4; i++) {
        tx_buffers[i] = tx_buf[i];
    }
    
    // Set receive buffer address
    outl(rtl8139_io_base + RTL8139_RXBUF, (uint32_t)rx_buffer);
    
    // Reset receive buffer pointer (CAPR)
    outw(rtl8139_io_base + RTL8139_RXBUFPTR, 0);
    rx_offset = 0;
    
    // Set IMR (interrupt mask) - enable ROK and TOK
    outw(rtl8139_io_base + RTL8139_IMR, RTL8139_INT_ROK | RTL8139_INT_TOK);
    
    // Set RCR (receive config) - accept all packets with wrap
    // WRAP=1, accept all physical, multicast, broadcast
    outl(rtl8139_io_base + RTL8139_RCR, 
         0x0000000F |  // Accept all packets (AAP, APM, AM, AB)
         (1 << 7));     // WRAP at end of buffer
    
    // Set TCR (transmit config) - standard configuration
    outl(rtl8139_io_base + RTL8139_TCR, 0x03000700);
    
    // Enable receive and transmit
    outb(rtl8139_io_base + RTL8139_CMD, RTL8139_CMD_RX_EN | RTL8139_CMD_TX_EN);
    
    vga_print("RTL8139 READY!\n");
    vga_print("MAC Address: ");
    for (int i = 0; i < 6; i++) {
        uint8_t mac_byte = inb(rtl8139_io_base + RTL8139_IDR0 + i);
        if (mac_byte < 0x10) vga_putchar('0');
        vga_print_hex(mac_byte);
        if (i < 5) vga_putchar(':');
    }
    vga_print("\n");
    
    // Test: Send a dummy packet to make sure TX works
    vga_print("Network card initialized and ready to receive\n");
}

void rtl8139_send_packet(const uint8_t* data, uint16_t length) {
    if (rtl8139_io_base == 0 || length > 1500) return;
    
    // Copy to TX buffer
    for (uint16_t i = 0; i < length; i++) {
        tx_buffers[tx_current][i] = data[i];
    }
    
    // Set transmit address
    outl(rtl8139_io_base + RTL8139_TXADDR0 + (tx_current * 4), 
         (uint32_t)tx_buffers[tx_current]);
    
    // Set transmit status (length)
    outl(rtl8139_io_base + RTL8139_TXSTATUS0 + (tx_current * 4), length);
    
    // Move to next TX buffer
    tx_current = (tx_current + 1) % 4;
}

void rtl8139_handle_interrupt(void) {
    if (rtl8139_io_base == 0) return;
    
    uint16_t status = inb(rtl8139_io_base + RTL8139_ISR);
    
    // Acknowledge interrupt if any
    if (status != 0) {
        outb(rtl8139_io_base + RTL8139_ISR, status);
    }
    
    // Check if receive buffer is not empty (poll mode)
    // Bit 0 of CMD register = 1 means buffer is empty
    uint8_t cmd = inb(rtl8139_io_base + RTL8139_CMD);
    
    if ((cmd & 0x01) == 0) {
        extern void net_process_packet(uint8_t* packet, uint16_t length);
        
        uint16_t* header = (uint16_t*)(rx_buffer + rx_offset);
        uint16_t rx_status = header[0];
        uint16_t length = header[1];
        
        // Debug: show that we received something
        static int rx_count = 0;
        if (rx_count++ < 5) {
            vga_print("RX: status=");
            vga_print_hex(rx_status);
            vga_print(" len=");
            vga_print_hex(length);
            vga_print("\n");
        }
        
        // Check if packet is valid (ROK bit set in status)
        if ((rx_status & 0x01) && length > 0 && length < 1500) {
            // Process packet (skip 4-byte header)
            net_process_packet(rx_buffer + rx_offset + 4, length - 4);
        }
        
        // Move offset (align to 4 bytes)
        rx_offset = (rx_offset + length + 4 + 3) & ~3;
        if (rx_offset >= 8192) rx_offset -= 8192;
        
        // Update CAPR (Current Address of Packet Read)
        outw(rtl8139_io_base + RTL8139_RXBUFPTR, rx_offset - 0x10);
    }
}

uint8_t rtl8139_get_mac(uint8_t index) {
    if (rtl8139_io_base == 0 || index >= 6) return 0;
    return inb(rtl8139_io_base + RTL8139_IDR0 + index);
}
