/*
 * ==============================================================================
 * MEMORY MANAGER IMPLEMENTATION
 * ==============================================================================
 */

#include "include/memory.h"
#include "include/string.h"

/* Memory bitmap 
 * WHY: Each bit represents one 4KB page (1=used, 0=free)
 * SIZE: 32MB / 4KB = 8192 pages = 8192 bits = 1024 bytes */
static uint8_t memory_bitmap[NUM_PAGES / 8];

/* Statistics */
static uint32_t pages_used = 0;

/*
 * set_page_used() - Mark a page as used in bitmap
 * 
 * WHAT: Set the bit for a specific page to 1
 * WHY: To track which pages are allocated
 * HOW: Calculate byte and bit position, use OR to set bit
 */
static void set_page_used(uint32_t page_num) 
{
    uint32_t byte = page_num / 8;       /* Which byte in bitmap */
    uint32_t bit = page_num % 8;        /* Which bit in that byte */
    memory_bitmap[byte] |= (1 << bit);  /* Set the bit to 1 */
}

/*
 * set_page_free() - Mark a page as free in bitmap
 * 
 * WHAT: Set the bit for a specific page to 0
 * WHY: To mark pages as available for allocation
 * HOW: Calculate byte and bit position, use AND with NOT to clear bit
 */
static void set_page_free(uint32_t page_num) 
{
    uint32_t byte = page_num / 8;
    uint32_t bit = page_num % 8;
    memory_bitmap[byte] &= ~(1 << bit);  /* Clear the bit to 0 */
}

/*
 * is_page_used() - Check if a page is used
 * 
 * WHAT: Read the bit for a specific page
 * WHY: To check availability before allocating
 * HOW: Calculate position and use AND to test bit
 * RETURNS: 1 if used, 0 if free
 */
static int is_page_used(uint32_t page_num) 
{
    uint32_t byte = page_num / 8;
    uint32_t bit = page_num % 8;
    return (memory_bitmap[byte] & (1 << bit)) != 0;
}

/*
 * memory_init() - Initialize memory manager
 * 
 * WHAT: Set up the page bitmap and mark kernel memory as used
 * WHY: Must be called before any memory allocation
 * HOW: Clear bitmap, then mark first 1MB as used (kernel code lives there)
 */
void memory_init(void) 
{
    uint32_t i;
    
    /* Clear entire bitmap (mark all pages as free) */
    memset(memory_bitmap, 0, sizeof(memory_bitmap));
    
    /* Mark first 1MB as used (bootloader, kernel, VGA memory, etc.)
     * WHY: Our kernel is loaded at 0x10000 (64KB) and grows from there
     *      Also protects BIOS data area, VGA buffer, etc. */
    for (i = 0; i < (1024 * 1024) / PAGE_SIZE; i++) {
        set_page_used(i);
        pages_used++;
    }
}

/*
 * alloc_page() - Allocate a 4KB page of memory
 * 
 * WHAT: Find a free page and mark it as used
 * WHY: When kernel or programs need memory
 * HOW: Scan bitmap for first free page, mark it used, return address
 * RETURNS: Pointer to start of page, or NULL if no memory available
 */
void* alloc_page(void) 
{
    uint32_t page_num;
    
    /* Search for a free page */
    for (page_num = 0; page_num < NUM_PAGES; page_num++) {
        if (!is_page_used(page_num)) {
            /* Found a free page! */
            set_page_used(page_num);
            pages_used++;
            
            /* Calculate physical address of this page
             * WHY: page_num * PAGE_SIZE gives us the byte address */
            return (void*)(page_num * PAGE_SIZE);
        }
    }
    
    /* No free pages available */
    return NULL;
}

/*
 * free_page() - Free a previously allocated page
 * 
 * WHAT: Mark a page as available again
 * WHY: To reclaim memory that's no longer needed
 * HOW: Convert address to page number, clear bitmap bit
 */
void free_page(void* page) 
{
    /* Convert address to page number */
    uint32_t page_num = (uint32_t)page / PAGE_SIZE;
    
    /* Sanity check: is this a valid page? */
    if (page_num >= NUM_PAGES) {
        return;  /* Invalid address */
    }
    
    /* Mark as free */
    if (is_page_used(page_num)) {
        set_page_free(page_num);
        pages_used--;
    }
}

/*
 * get_free_memory() - Get amount of free memory in bytes
 * 
 * WHAT: Calculate how much RAM is still available
 * WHY: For reporting system status
 * HOW: (Total pages - Used pages) * Page size
 */
uint32_t get_free_memory(void) 
{
    return (NUM_PAGES - pages_used) * PAGE_SIZE;
}
