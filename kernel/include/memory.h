/*
 * ==============================================================================
 * MEMORY MANAGEMENT
 * ==============================================================================
 * WHAT: System for tracking and allocating physical memory
 * WHY: The OS needs to know which RAM is free and which is used
 * HOW: Use a bitmap where each bit represents one 4KB page of memory
 * 
 * TECHNICAL DETAILS:
 * - Page size: 4KB (4096 bytes) - standard x86 page size
 * - We use a bitmap: 1 bit per page (1 = used, 0 = free)
 * - For 32MB RAM: 32MB / 4KB = 8192 pages = 1024 bytes of bitmap
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

/* Memory constants */
#define PAGE_SIZE 4096              /* 4KB pages */
#define MEMORY_SIZE (32 * 1024 * 1024)  /* Assume 32MB for now */
#define NUM_PAGES (MEMORY_SIZE / PAGE_SIZE)

/* Function declarations */
void memory_init(void);
void* alloc_page(void);
void free_page(void* page);
uint32_t get_free_memory(void);

#endif /* MEMORY_H */
