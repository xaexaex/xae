/*
 * ==============================================================================
 * XAE FILESYSTEM IMPLEMENTATION
 * ==============================================================================
 */

#include "include/xaefs.h"
#include "include/memory.h"
#include "include/string.h"
#include "include/vga.h"

/* In-memory filesystem structures */
static struct xaefs_superblock superblock;
static struct xaefs_inode inode_table[XAEFS_MAX_FILES];
static struct xaefs_file file_table[16];  /* Max 16 open files */

/* Filesystem state */
static uint8_t fs_initialized = 0;

/*
 * xaefs_init() - Initialize the filesystem
 * 
 * WHAT: Set up the filesystem in memory
 * WHY: Must be called before any filesystem operations
 * HOW: Initialize superblock and inode table
 */
void xaefs_init(void) 
{
    /* Clear all structures */
    memset(&superblock, 0, sizeof(superblock));
    memset(inode_table, 0, sizeof(inode_table));
    memset(file_table, 0, sizeof(file_table));
    
    /* Set up superblock */
    superblock.magic = 0x58414546;  /* "XAEF" in hex */
    superblock.version = 1;
    superblock.block_size = XAEFS_BLOCK_SIZE;
    superblock.total_blocks = 1024;  /* 4MB filesystem for now */
    superblock.free_blocks = 1024 - 10;  /* Reserve first 10 for metadata */
    superblock.total_inodes = XAEFS_MAX_FILES;
    superblock.free_inodes = XAEFS_MAX_FILES - 1;  /* -1 for root dir */
    
    /* Create root directory (inode 0)
     * WHY: Every filesystem needs a root directory (/) */
    inode_table[0].inode_num = 0;
    inode_table[0].parent_inode = 0;  /* Root's parent is itself */
    inode_table[0].size = 0;
    inode_table[0].type = XAEFS_FILE_DIRECTORY;
    inode_table[0].priority = XAEFS_PRIORITY_CRITICAL;
    inode_table[0].version = 1;
    inode_table[0].name[0] = '/';
    inode_table[0].name[1] = '\0';
    
    fs_initialized = 1;
    
    vga_print("  - Filesystem magic: 0x58414546\n");
    vga_print("  - Block size: 4096 bytes\n");
    vga_print("  - Total capacity: 4 MB\n");
}

/*
 * xaefs_format() - Format a disk with XAE-FS
 * 
 * WHAT: Prepare a disk to use our filesystem
 * WHY: To create a new filesystem on empty storage
 * HOW: Write superblock and empty inode table
 */
void xaefs_format(const char* label) 
{
    uint32_t i;
    
    /* Copy volume label */
    for (i = 0; i < 31 && label[i] != '\0'; i++) {
        superblock.label[i] = label[i];
    }
    superblock.label[i] = '\0';
    
    vga_print("  - Volume label: ");
    vga_print(superblock.label);
    vga_print("\n");
}

/*
 * find_free_inode() - Find an unused inode
 * 
 * WHAT: Search for an available inode slot
 * WHY: Needed when creating new files
 * HOW: Scan inode table for empty entry
 * RETURNS: Inode number, or -1 if none available
 */
static int find_free_inode(void) 
{
    uint32_t i;
    
    for (i = 1; i < XAEFS_MAX_FILES; i++) {  /* Start at 1 (0 is root) */
        if (inode_table[i].inode_num == 0) {
            return i;
        }
    }
    
    return -1;  /* No free inodes */
}

/*
 * xaefs_create() - Create a new file
 * 
 * WHAT: Create a new file or directory
 * WHY: Basic filesystem operation
 * HOW: Allocate inode, fill metadata, mark as used
 * RETURNS: Inode number, or -1 on error
 */
int xaefs_create(const char* path, uint8_t type, uint8_t priority) 
{
    int inode_num;
    struct xaefs_inode* inode;
    uint32_t i;
    
    if (!fs_initialized) return -1;
    
    /* Find free inode */
    inode_num = find_free_inode();
    if (inode_num < 0) return -1;
    
    /* Set up inode */
    inode = &inode_table[inode_num];
    inode->inode_num = inode_num;
    inode->parent_inode = 0;  /* For now, everything goes in root */
    inode->size = 0;
    inode->type = type;
    inode->priority = priority;
    inode->version = 1;
    inode->tag_count = 0;
    
    /* Copy filename */
    for (i = 0; i < XAEFS_MAX_FILENAME - 1 && path[i] != '\0'; i++) {
        inode->name[i] = path[i];
    }
    inode->name[i] = '\0';
    
    superblock.free_inodes--;
    
    return inode_num;
}

/*
 * xaefs_mkdir() - Create a directory
 * 
 * WHAT: Create a new directory
 * WHY: Convenience wrapper around create()
 * HOW: Call create() with directory type
 */
int xaefs_mkdir(const char* path, uint8_t priority) 
{
    return xaefs_create(path, XAEFS_FILE_DIRECTORY, priority);
}

/*
 * xaefs_add_tag() - Add a tag to a file (UNIQUE FEATURE!)
 * 
 * WHAT: Attach a metadata tag to a file
 * WHY: To categorize files beyond just directories
 * HOW: Add tag string to inode's tag array
 * EXAMPLE: xaefs_add_tag("/myfile.txt", "important")
 */
int xaefs_add_tag(const char* path, const char* tag) 
{
    uint32_t i, j;
    struct xaefs_inode* inode = NULL;
    
    /* Find file by name */
    for (i = 0; i < XAEFS_MAX_FILES; i++) {
        if (inode_table[i].inode_num != 0) {
            /* Simple name comparison (in real FS, we'd parse path) */
            uint32_t match = 1;
            for (j = 0; path[j] != '\0' && inode_table[i].name[j] != '\0'; j++) {
                if (path[j] != inode_table[i].name[j]) {
                    match = 0;
                    break;
                }
            }
            if (match && path[j] == '\0' && inode_table[i].name[j] == '\0') {
                inode = &inode_table[i];
                break;
            }
        }
    }
    
    if (!inode) return -1;  /* File not found */
    if (inode->tag_count >= XAEFS_MAX_TAGS) return -1;  /* Too many tags */
    
    /* Add tag */
    for (i = 0; i < XAEFS_TAG_LENGTH - 1 && tag[i] != '\0'; i++) {
        inode->tags[inode->tag_count][i] = tag[i];
    }
    inode->tags[inode->tag_count][i] = '\0';
    inode->tag_count++;
    
    return 0;
}

/*
 * xaefs_set_priority() - Change file priority (UNIQUE FEATURE!)
 * 
 * WHAT: Set the priority level of a file
 * WHY: Can be used by scheduler or backup systems
 * HOW: Update priority field in inode
 */
int xaefs_set_priority(const char* path, uint8_t priority) 
{
    uint32_t i, j;
    
    /* Find file */
    for (i = 0; i < XAEFS_MAX_FILES; i++) {
        if (inode_table[i].inode_num != 0) {
            uint32_t match = 1;
            for (j = 0; path[j] != '\0' && inode_table[i].name[j] != '\0'; j++) {
                if (path[j] != inode_table[i].name[j]) {
                    match = 0;
                    break;
                }
            }
            if (match && path[j] == '\0' && inode_table[i].name[j] == '\0') {
                inode_table[i].priority = priority;
                return 0;
            }
        }
    }
    
    return -1;  /* File not found */
}

/*
 * xaefs_find_by_tag() - Find files by tag (UNIQUE FEATURE!)
 * 
 * WHAT: Search for all files with a specific tag
 * WHY: To use the tagging system for file discovery
 * HOW: Scan all inodes, check their tags, print matches
 */
void xaefs_find_by_tag(const char* tag) 
{
    uint32_t i, j, k;
    uint8_t found = 0;
    
    vga_print("\nFiles tagged with '");
    vga_print(tag);
    vga_print("':\n");
    
    /* Loop through all inodes */
    for (i = 1; i < XAEFS_MAX_FILES; i++) {  /* Skip root at 0 */
        if (inode_table[i].inode_num != 0) {
            /* Check each tag in this file */
            for (j = 0; j < inode_table[i].tag_count; j++) {
                /* Compare tag strings */
                uint8_t match = 1;
                for (k = 0; tag[k] != '\0'; k++) {
                    if (tag[k] != inode_table[i].tags[j][k]) {
                        match = 0;
                        break;
                    }
                }
                
                if (match && inode_table[i].tags[j][k] == '\0') {
                    /* Found a match! */
                    vga_print("  - ");
                    vga_print(inode_table[i].name);
                    vga_print("\n");
                    found = 1;
                    break;
                }
            }
        }
    }
    
    if (!found) {
        vga_print("  (no files found)\n");
    }
}

/*
 * xaefs_list_dir() - List files in a directory
 * 
 * WHAT: Show all files in the filesystem
 * WHY: So users can see what files exist
 * HOW: Print all inodes in the root directory
 */
int xaefs_list_dir(const char* path) 
{
    uint32_t i, j;
    const char* type_names[] = {"FILE", "DIR ", "DEV "};
    const char* priority_names[] = {"LOW ", "NORM", "HIGH", "CRIT"};
    
    vga_print("\nFiles in ");
    vga_print(path);
    vga_print(":\n");
    vga_print("NAME                  TYPE  PRIORITY  SIZE    TAGS\n");
    vga_print("----------------------------------------------------\n");
    
    for (i = 1; i < XAEFS_MAX_FILES; i++) {  /* Skip root at 0 */
        if (inode_table[i].inode_num != 0) {
            /* Print filename */
            vga_print(inode_table[i].name);
            
            /* Pad to 22 chars */
            for (j = strlen(inode_table[i].name); j < 22; j++) {
                vga_putchar(' ');
            }
            
            /* Print type */
            vga_print(type_names[inode_table[i].type]);
            vga_print("  ");
            
            /* Print priority */
            vga_print(priority_names[inode_table[i].priority]);
            vga_print("      ");
            
            /* Print size (simplified) */
            vga_print("0 KB");
            vga_print("    ");
            
            /* Print tags */
            if (inode_table[i].tag_count > 0) {
                vga_putchar('[');
                for (j = 0; j < inode_table[i].tag_count; j++) {
                    if (j > 0) vga_print(", ");
                    vga_print(inode_table[i].tags[j]);
                }
                vga_putchar(']');
            }
            
            vga_putchar('\n');
        }
    }
    
    return 0;
}
