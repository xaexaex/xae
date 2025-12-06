/*
 * ==============================================================================
 * ATA/IDE DISK DRIVER
 * ==============================================================================
 * WHAT: Driver for reading/writing hard disk sectors
 * WHY: To persist files to actual storage
 * HOW: Use ATA PIO (Programmed I/O) mode to access disk
 * 
 * NOTES:
 * - ATA = Advanced Technology Attachment (standard hard disk interface)
 * - PIO = Programmed I/O (CPU directly controls transfers)
 * - Each sector is 512 bytes
 * - We use LBA28 addressing (can address up to 128GB)
 */

#ifndef DISK_H
#define DISK_H

#include <stdint.h>

/* Disk constants */
#define DISK_SECTOR_SIZE 512

/* Disk functions */
void disk_init(void);
int disk_read_sector(uint32_t lba, uint8_t* buffer);
int disk_write_sector(uint32_t lba, const uint8_t* buffer);
int disk_read_sectors(uint32_t lba, uint32_t count, uint8_t* buffer);
int disk_write_sectors(uint32_t lba, uint32_t count, const uint8_t* buffer);

#endif /* DISK_H */
