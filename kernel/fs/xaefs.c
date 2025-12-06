/*
 * ==============================================================================
 * XAE FILESYSTEM IMPLEMENTATION
 * ==============================================================================
 */

#include "include/xaefs.h"
#include "include/memory.h"
#include "include/string.h"
#include "include/vga.h"
#include "include/disk.h"

/* Disk layout for XAE-FS:
 * Sector 0: Bootloader (reserved)
 * Sector 1: Superblock
 * Sector 2-9: Inode table (256 inodes * ~100 bytes each = ~25KB = 8 sectors)
 * Sector 10+: File data blocks
 */
#define XAEFS_SUPERBLOCK_SECTOR 1
#define XAEFS_INODE_TABLE_SECTOR 2
#define XAEFS_INODE_TABLE_SECTORS 8
#define XAEFS_DATA_START_SECTOR 10

/* In-memory filesystem structures */
static struct xaefs_superblock superblock;
static struct xaefs_inode inode_table[XAEFS_MAX_FILES];
static struct xaefs_file file_table[16];  /* Max 16 open files */

/* Filesystem state */
static uint8_t fs_initialized = 0;
static uint8_t auto_sync_enabled = 1;  /* Auto-save on every change for production */

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
    
    /* Create unique starter hierarchy */
    vga_print("  - Creating XAE hierarchy: /sys /usr /tmp\n");
    
    /* Create /sys (system files) */
    int idx = xaefs_create("sys", XAEFS_FILE_DIRECTORY, XAEFS_PRIORITY_HIGH);
    if (idx >= 0) inode_table[idx].parent_inode = 0;
    
    /* Create /usr (user files) */
    idx = xaefs_create("usr", XAEFS_FILE_DIRECTORY, XAEFS_PRIORITY_NORMAL);
    if (idx >= 0) inode_table[idx].parent_inode = 0;
    
    /* Create /tmp (temporary) */
    idx = xaefs_create("tmp", XAEFS_FILE_DIRECTORY, XAEFS_PRIORITY_LOW);
    if (idx >= 0) inode_table[idx].parent_inode = 0;
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
 * find_file_by_name() - Find a file by name (global search)
 * 
 * WHAT: Search for a file in the inode table
 * WHY: Common operation for all file commands
 * HOW: Compare name strings in all inodes
 * RETURNS: Pointer to inode, or NULL if not found
 */
static struct xaefs_inode* find_file_by_name(const char* name) 
{
    uint32_t i, j;
    
    for (i = 0; i < XAEFS_MAX_FILES; i++) {
        if (inode_table[i].inode_num != 0) {
            /* Compare name */
            uint32_t match = 1;
            for (j = 0; name[j] != '\0' && inode_table[i].name[j] != '\0'; j++) {
                if (name[j] != inode_table[i].name[j]) {
                    match = 0;
                    break;
                }
            }
            if (match && name[j] == '\0' && inode_table[i].name[j] == '\0') {
                return &inode_table[i];
            }
        }
    }
    
    return NULL;  /* Not found */
}

/*
 * find_file_in_dir() - Find a file in specific directory
 * 
 * WHAT: Search for a file by name within a parent directory
 * WHY: To support proper directory hierarchy
 * HOW: Match both name AND parent_inode
 * RETURNS: Pointer to inode, or NULL if not found
 */
static struct xaefs_inode* find_file_in_dir(const char* name, uint32_t parent_inode) 
{
    uint32_t i, j;
    
    for (i = 0; i < XAEFS_MAX_FILES; i++) {
        if (inode_table[i].inode_num != 0 && inode_table[i].parent_inode == parent_inode) {
            /* Compare name */
            uint32_t match = 1;
            for (j = 0; name[j] != '\0' && inode_table[i].name[j] != '\0'; j++) {
                if (name[j] != inode_table[i].name[j]) {
                    match = 0;
                    break;
                }
            }
            if (match && name[j] == '\0' && inode_table[i].name[j] == '\0') {
                return &inode_table[i];
            }
        }
    }
    
    return NULL;  /* Not found */
}

/*
 * find_parent_dir() - Find parent directory inode from path
 * 
 * WHAT: Extract and find parent directory
 * WHY: To support directory hierarchy
 * HOW: Parse path and find parent dir inode
 * RETURNS: Parent inode number (0 for root)
 */
static uint32_t find_parent_dir(const char* current_dir) 
{
    uint32_t i;
    
    /* If at root, parent is root */
    if (strcmp(current_dir, "/") == 0) {
        return 0;
    }
    
    /* Extract directory name from path */
    /* For "/usr" we want "usr", for "/sys/bin" we want "bin" */
    const char* dir_name = current_dir;
    if (current_dir[0] == '/') {
        dir_name = current_dir + 1;  /* Skip leading / */
    }
    
    /* For now, only support single-level: /dirname */
    /* Find last / if any (for multi-level support later) */
    uint32_t last_slash = 0;
    for (i = 0; dir_name[i] != '\0'; i++) {
        if (dir_name[i] == '/') last_slash = i;
    }
    if (last_slash > 0) {
        dir_name = dir_name + last_slash + 1;
    }
    
    /* Find directory by name */
    struct xaefs_inode* dir = find_file_by_name(dir_name);
    if (dir && dir->type == XAEFS_FILE_DIRECTORY) {
        return dir->inode_num;
    }
    
    return 0;  /* Default to root */
}

/*
 * xaefs_create() - Create a new file
 * 
 * WHAT: Create a new file or directory
 * WHY: Basic filesystem operation
 * HOW: Allocate inode, fill metadata, mark as used
 * RETURNS: Inode number, or negative error code
 *   -1: filesystem not initialized
 *   -2: no free inodes (filesystem full)
 *   -3: file already exists
 */
int xaefs_create(const char* path, uint8_t type, uint8_t priority) 
{
    int inode_num;
    struct xaefs_inode* inode;
    uint32_t i;
    
    if (!fs_initialized) return -1;
    
    /* Check if file already exists */
    if (find_file_by_name(path) != NULL) return -3;
    
    /* Find free inode */
    inode_num = find_free_inode();
    if (inode_num < 0) return -2;
    
    /* Set up inode */
    inode = &inode_table[inode_num];
    inode->inode_num = inode_num;
    inode->parent_inode = 0;  /* Will be updated by shell based on current_path */
    inode->size = 0;
    inode->type = type;
    inode->priority = priority;
    inode->version = 1;
    inode->tag_count = 0;
    
    /* Extract just the filename from path (e.g., "new.txt" from "sys/new.txt") */
    const char* filename = path;
    for (i = 0; path[i] != '\0'; i++) {
        if (path[i] == '/') {
            filename = &path[i + 1];  /* Point to char after last / */
        }
    }
    
    /* Copy filename only */
    for (i = 0; i < XAEFS_MAX_FILENAME - 1 && filename[i] != '\0'; i++) {
        inode->name[i] = filename[i];
    }
    inode->name[i] = '\0';
    
    superblock.free_inodes--;
    
    /* Auto-sync to disk */
    if (auto_sync_enabled) {
        xaefs_sync();
    }
    
    return inode_num;
}

/*
 * xaefs_set_parent() - Set parent directory for a file
 * 
 * WHAT: Update the parent_inode field
 * WHY: To build directory hierarchy
 * HOW: Find file and parent, update link
 */
int xaefs_set_parent(const char* filename, const char* parent_path) 
{
    uint32_t i;
    
    /* Extract just the filename from full path */
    const char* base_name = filename;
    for (i = 0; filename[i] != '\0'; i++) {
        if (filename[i] == '/') {
            base_name = &filename[i + 1];
        }
    }
    
    struct xaefs_inode* file = find_file_by_name(base_name);
    if (!file) return -1;
    
    file->parent_inode = find_parent_dir(parent_path);
    return 0;
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
    uint32_t i;
    struct xaefs_inode* inode;
    
    /* Find file by name */
    inode = find_file_by_name(path);
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
 * xaefs_delete() - Delete a file (global search)
 * 
 * WHAT: Remove a file from the filesystem
 * WHY: Basic file operation
 * HOW: Clear the inode and free it
 * RETURNS: 0 on success, -1 on error
 */
int xaefs_delete(const char* path) 
{
    struct xaefs_inode* inode;
    
    /* Find file */
    inode = find_file_by_name(path);
    if (!inode) return -1;  /* File not found */
    
    /* Don't allow deleting root */
    if (inode->inode_num == 0) return -1;
    
    /* Clear the inode */
    memset(inode, 0, sizeof(struct xaefs_inode));
    superblock.free_inodes++;
    
    return 0;
}

/*
 * xaefs_delete_in_dir() - Delete a file in specific directory
 * 
 * WHAT: Remove a file from current directory
 * WHY: To support directory hierarchy in shell
 * HOW: Find file by name+parent, then delete
 * RETURNS: 0 on success, -1 on error
 */
int xaefs_delete_in_dir(const char* name, const char* current_dir) 
{
    struct xaefs_inode* inode;
    uint32_t parent_num;
    
    /* Get parent directory inode */
    parent_num = find_parent_dir(current_dir);
    
    /* Find file in this directory */
    inode = find_file_in_dir(name, parent_num);
    if (!inode) return -1;  /* File not found */
    
    /* Don't allow deleting root or directories */
    if (inode->inode_num == 0) return -1;
    
    /* Clear the inode */
    memset(inode, 0, sizeof(struct xaefs_inode));
    superblock.free_inodes++;
    
    /* Auto-sync to disk */
    if (auto_sync_enabled) {
        xaefs_sync();
    }
    
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
    struct xaefs_inode* inode;
    
    /* Find file */
    inode = find_file_by_name(path);
    if (!inode) return -1;  /* File not found */
    
    inode->priority = priority;
    return 0;
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
 * WHAT: Show files in specified directory
 * WHY: So users can see directory contents
 * HOW: Print inodes that have matching parent_inode
 */
int xaefs_list_dir(const char* path) 
{
    uint32_t i, j;
    uint32_t parent_num;
    const char* type_names[] = {"FILE", "DIR ", "DEV "};
    const char* priority_names[] = {"LOW ", "NORM", "HIGH", "CRIT"};
    uint8_t found_any = 0;
    
    /* Find parent directory inode number */
    parent_num = find_parent_dir(path);
    
    vga_print("\nFiles in ");
    vga_print(path);
    vga_print(":\n");
    vga_print("NAME                  TYPE  PRIORITY  SIZE    TAGS\n");
    vga_print("----------------------------------------------------\n");
    
    /* Show files that have this directory as parent */
    for (i = 1; i < XAEFS_MAX_FILES; i++) {  /* Skip root at 0 */
        if (inode_table[i].inode_num != 0 && inode_table[i].parent_inode == parent_num) {
            found_any = 1;
            
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
    
    if (!found_any) {
        vga_print("(empty directory)\n");
    }
    
    return 0;
}

/*
 * ==============================================================================
 * DISK PERSISTENCE FUNCTIONS
 * ==============================================================================
 */

/*
 * xaefs_sync() - Save filesystem to disk
 * 
 * WHAT: Write all filesystem data to persistent storage
 * WHY: So files survive power-off
 * HOW: Write superblock and inode table to disk sectors
 */
void xaefs_sync(void) 
{
    uint8_t buffer[DISK_SECTOR_SIZE];
    uint32_t i;
    
    /* Write superblock to sector 1 */
    memset(buffer, 0, DISK_SECTOR_SIZE);
    memcpy(buffer, &superblock, sizeof(superblock));
    if (disk_write_sector(XAEFS_SUPERBLOCK_SECTOR, buffer) != 0) {
        vga_print("[ERROR] Failed to write superblock to disk\n");
        return;
    }
    
    /* Write inode table (8 sectors) */
    for (i = 0; i < XAEFS_INODE_TABLE_SECTORS; i++) {
        uint32_t inodes_per_sector = DISK_SECTOR_SIZE / sizeof(struct xaefs_inode);
        uint32_t inode_start = i * inodes_per_sector;
        
        memset(buffer, 0, DISK_SECTOR_SIZE);
        
        /* Copy inodes to buffer */
        uint32_t j;
        for (j = 0; j < inodes_per_sector && (inode_start + j) < XAEFS_MAX_FILES; j++) {
            memcpy(buffer + (j * sizeof(struct xaefs_inode)), 
                   &inode_table[inode_start + j], 
                   sizeof(struct xaefs_inode));
        }
        
        if (disk_write_sector(XAEFS_INODE_TABLE_SECTOR + i, buffer) != 0) {
            vga_print("[ERROR] Failed to write inode table to disk\n");
            return;
        }
    }
    
    /* Show sync status */
    if (auto_sync_enabled) {
        vga_print("  [Synced to disk]\n");
    }
}

/*
 * xaefs_load() - Load filesystem from disk
 * 
 * WHAT: Read filesystem data from persistent storage
 * WHY: To restore files after boot
 * HOW: Read superblock and inode table from disk sectors
 */
void xaefs_load(void) 
{
    uint8_t buffer[DISK_SECTOR_SIZE];
    uint32_t i;
    
    vga_print("  - Attempting to load filesystem from disk...\n");
    
    /* Clear inode table before loading */
    memset(inode_table, 0, sizeof(inode_table));
    
    /* Read superblock from sector 1 */
    if (disk_read_sector(XAEFS_SUPERBLOCK_SECTOR, buffer) != 0) {
        vga_print("  - Disk read failed, will create new filesystem\n");
        return;
    }
    
    /* Check magic number */
    struct xaefs_superblock* sb = (struct xaefs_superblock*)buffer;
    if (sb->magic != 0x58414546) {
        vga_print("  - No valid XAE-FS found on disk\n");
        return;
    }
    
    /* Load superblock */
    memcpy(&superblock, buffer, sizeof(superblock));
    vga_print("  - Found existing XAE-FS! Loading...\n");
    
    /* Read inode table (8 sectors) */
    for (i = 0; i < XAEFS_INODE_TABLE_SECTORS; i++) {
        if (disk_read_sector(XAEFS_INODE_TABLE_SECTOR + i, buffer) != 0) {
            vga_print("  - Error reading inode table, aborting load\n");
            fs_initialized = 0;
            return;
        }
        
        /* Copy inodes from buffer */
        uint32_t inodes_per_sector = DISK_SECTOR_SIZE / sizeof(struct xaefs_inode);
        uint32_t inode_start = i * inodes_per_sector;
        uint32_t j;
        
        for (j = 0; j < inodes_per_sector && (inode_start + j) < XAEFS_MAX_FILES; j++) {
            memcpy(&inode_table[inode_start + j],
                   buffer + (j * sizeof(struct xaefs_inode)),
                   sizeof(struct xaefs_inode));
        }
    }
    
    vga_print("  - Filesystem restored from disk!\n");
    vga_print("  - Loaded ");
    char num_str[16];
    uint32_t file_count = 0;
    for (i = 0; i < XAEFS_MAX_FILES; i++) {
        if (inode_table[i].inode_num != 0) file_count++;
    }
    /* Simple number printing */
    if (file_count < 10) {
        vga_putchar('0' + file_count);
    } else {
        vga_putchar('0' + (file_count / 10));
        vga_putchar('0' + (file_count % 10));
    }
    vga_print(" files from disk\n");
    
    fs_initialized = 1;
}

/*
 * xaefs_is_loaded() - Check if filesystem was successfully loaded
 */
uint8_t xaefs_is_loaded(void) 
{
    return fs_initialized;
}

/*
 * xaefs_debug_list_all() - Show all inodes with parent info (DEBUG)
 */
void xaefs_debug_list_all(void)
{
    uint32_t i;
    
    vga_print("\n=== DEBUG: ALL INODES ===\n");
    vga_print("ID  NAME            PARENT  TYPE\n");
    vga_print("-----------------------------------\n");
    
    for (i = 0; i < XAEFS_MAX_FILES; i++) {
        if (inode_table[i].inode_num != 0) {
            /* Print inode number */
            if (i < 10) {
                vga_putchar('0' + i);
            } else {
                vga_putchar('0' + (i / 10));
                vga_putchar('0' + (i % 10));
            }
            vga_print("  ");
            
            /* Print name */
            vga_print(inode_table[i].name);
            uint32_t j;
            for (j = strlen(inode_table[i].name); j < 16; j++) {
                vga_putchar(' ');
            }
            
            /* Print parent */
            uint32_t p = inode_table[i].parent_inode;
            if (p < 10) {
                vga_putchar('0' + p);
            } else {
                vga_putchar('0' + (p / 10));
                vga_putchar('0' + (p % 10));
            }
            vga_print("      ");
            
            /* Print type */
            if (inode_table[i].type == XAEFS_FILE_DIRECTORY) {
                vga_print("DIR");
            } else {
                vga_print("FILE");
            }
            vga_print("\n");
        }
    }
    vga_print("\n");
}
