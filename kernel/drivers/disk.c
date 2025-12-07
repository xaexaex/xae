/*
 * ==============================================================================
 * ATA/IDE DISK DRIVER IMPLEMENTATION
 * ==============================================================================
 */

#include "include/disk.h"
#include "include/vga.h"

/* ATA I/O Ports (Primary bus, Secondary drive) */
#define ATA_DATA        0x1F0   /* Data register (16-bit) */
#define ATA_ERROR       0x1F1   /* Error register (read) */
#define ATA_FEATURES    0x1F1   /* Features register (write) */
#define ATA_SECTOR_COUNT 0x1F2  /* Sector count */
#define ATA_LBA_LOW     0x1F3   /* LBA low byte */
#define ATA_LBA_MID     0x1F4   /* LBA middle byte */
#define ATA_LBA_HIGH    0x1F5   /* LBA high byte */
#define ATA_DRIVE       0x1F6   /* Drive/Head register */
#define ATA_STATUS      0x1F7   /* Status register (read) */
#define ATA_COMMAND     0x1F7   /* Command register (write) */

/* Drive selection */
#define ATA_DRIVE_MASTER 0xE0  /* Master drive (LBA mode) */
#define ATA_DRIVE_SLAVE  0xF0  /* Slave drive (LBA mode) */
#define SELECTED_DRIVE   ATA_DRIVE_SLAVE  /* Use secondary disk for data */

/* ATA Status bits */
#define ATA_SR_BSY      0x80    /* Busy */
#define ATA_SR_DRDY     0x40    /* Drive ready */
#define ATA_SR_DRQ      0x08    /* Data request ready */
#define ATA_SR_ERR      0x01    /* Error */

/* ATA Commands */
#define ATA_CMD_READ    0x20    /* Read sectors */
#define ATA_CMD_WRITE   0x30    /* Write sectors */
#define ATA_CMD_IDENTIFY 0xEC   /* Identify drive */

/* Port I/O functions */
static inline uint8_t inb(uint16_t port) 
{
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) 
{
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) 
{
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) 
{
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

/*
 * disk_wait() - Wait for disk to be ready
 * 
 * WHAT: Poll status register until disk is ready
 * WHY: Disk operations take time, must wait
 * HOW: Check BSY and DRDY bits in status register
 */
static int disk_wait(void) 
{
    uint8_t status;
    uint32_t timeout = 100000;  /* Timeout counter */
    
    /* Wait while busy */
    while ((status = inb(ATA_STATUS)) & ATA_SR_BSY) {
        if (--timeout == 0) return -1;  /* Timeout */
    }
    
    timeout = 100000;
    
    /* Wait until ready */
    while (!((status = inb(ATA_STATUS)) & ATA_SR_DRDY)) {
        if (--timeout == 0) return -1;  /* Timeout */
    }
    
    return 0;
}

/*
 * disk_init() - Initialize disk driver
 * 
 * WHAT: Set up ATA disk for use
 * WHY: Must initialize before reading/writing
 * HOW: Select drive, check if present
 */
void disk_init(void) 
{
    vga_print("  - Initializing ATA disk driver...\n");
    
    /* Select slave drive (secondary disk for data storage) */
    outb(ATA_DRIVE, SELECTED_DRIVE);
    
    /* Small delay for drive to respond */
    uint32_t i;
    for (i = 0; i < 10000; i++) {
        __asm__ volatile ("nop");
    }
    
    /* Try to wait for drive - if it times out, continue anyway */
    if (disk_wait() != 0) {
        vga_print("  - Warning: Data disk not responding\n");
        vga_print("  - System will continue without persistence\n");
        return;
    }
    
    vga_print("  - Data disk ready (10MB persistent storage)\n");
}

/*
 * disk_read_sector() - Read one sector from disk
 * 
 * WHAT: Read 512 bytes from specified sector
 * WHY: To load data from persistent storage
 * HOW: Use ATA READ command in PIO mode
 * 
 * PARAMS:
 *   lba - Logical Block Address (sector number)
 *   buffer - Where to store the 512 bytes
 * RETURNS: 0 on success, -1 on error
 */
int disk_read_sector(uint32_t lba, uint8_t* buffer) 
{
    uint16_t i;
    uint16_t* buf16 = (uint16_t*)buffer;
    
    /* Wait for disk to be ready */
    if (disk_wait() != 0) return -1;
    
    /* Send read parameters */
    outb(ATA_SECTOR_COUNT, 1);              /* Read 1 sector */
    outb(ATA_LBA_LOW,  (uint8_t)(lba));     /* LBA bits 0-7 */
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8)); /* LBA bits 8-15 */
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16)); /* LBA bits 16-23 */
    outb(ATA_DRIVE, SELECTED_DRIVE | ((lba >> 24) & 0x0F)); /* LBA bits 24-27 */
    
    /* Send READ command */
    outb(ATA_COMMAND, ATA_CMD_READ);
    
    /* Wait for data to be ready */
    if (disk_wait() != 0) return -1;
    
    /* Check for errors */
    if (inb(ATA_STATUS) & ATA_SR_ERR) {
        return -1;
    }
    
    /* Read 256 words (512 bytes) */
    for (i = 0; i < 256; i++) {
        buf16[i] = inw(ATA_DATA);
    }
    
    return 0;
}

/*
 * disk_write_sector() - Write one sector to disk
 * 
 * WHAT: Write 512 bytes to specified sector
 * WHY: To save data to persistent storage
 * HOW: Use ATA WRITE command in PIO mode
 * 
 * PARAMS:
 *   lba - Logical Block Address (sector number)
 *   buffer - The 512 bytes to write
 * RETURNS: 0 on success, -1 on error
 */
int disk_write_sector(uint32_t lba, const uint8_t* buffer) 
{
    uint16_t i;
    const uint16_t* buf16 = (const uint16_t*)buffer;
    
    /* Wait for disk to be ready */
    if (disk_wait() != 0) return -1;
    
    /* Send write parameters */
    outb(ATA_SECTOR_COUNT, 1);              /* Write 1 sector */
    outb(ATA_LBA_LOW,  (uint8_t)(lba));     /* LBA bits 0-7 */
    outb(ATA_LBA_MID,  (uint8_t)(lba >> 8)); /* LBA bits 8-15 */
    outb(ATA_LBA_HIGH, (uint8_t)(lba >> 16)); /* LBA bits 16-23 */
    outb(ATA_DRIVE, SELECTED_DRIVE | ((lba >> 24) & 0x0F)); /* LBA bits 24-27 */
    
    /* Send WRITE command */
    outb(ATA_COMMAND, ATA_CMD_WRITE);
    
    /* Wait for disk to be ready for data */
    if (disk_wait() != 0) return -1;
    
    /* Write 256 words (512 bytes) */
    for (i = 0; i < 256; i++) {
        outw(ATA_DATA, buf16[i]);
    }
    
    /* Flush cache (wait for write to complete) */
    if (disk_wait() != 0) return -1;
    
    /* Check for errors */
    if (inb(ATA_STATUS) & ATA_SR_ERR) {
        return -1;
    }
    
    return 0;
}

/*
 * disk_read_sectors() - Read multiple sectors
 */
int disk_read_sectors(uint32_t lba, uint32_t count, uint8_t* buffer) 
{
    uint32_t i;
    
    for (i = 0; i < count; i++) {
        if (disk_read_sector(lba + i, buffer + (i * DISK_SECTOR_SIZE)) != 0) {
            return -1;
        }
    }
    
    return 0;
}

/*
 * disk_write_sectors() - Write multiple sectors
 */
int disk_write_sectors(uint32_t lba, uint32_t count, const uint8_t* buffer) 
{
    uint32_t i;
    
    for (i = 0; i < count; i++) {
        if (disk_write_sector(lba + i, buffer + (i * DISK_SECTOR_SIZE)) != 0) {
            return -1;
        }
    }
    
    return 0;
}
