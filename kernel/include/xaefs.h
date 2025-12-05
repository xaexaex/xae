/*
 * ==============================================================================
 * XAE FILESYSTEM (XAE-FS) - YOUR UNIQUE FILESYSTEM!
 * ==============================================================================
 * WHAT: A custom filesystem designed specifically for this OS
 * WHY: To organize files and directories on disk in our own way
 * HOW: Unlike FAT32 or ext4, we use a node-based structure
 * 
 * DESIGN PHILOSOPHY:
 * - Simple and educational (not production-ready)
 * - Tree-based structure (like most filesystems)
 * - Fixed-size blocks for simplicity
 * - In-memory for now (we'll add disk I/O later)
 * 
 * FILESYSTEM LAYOUT:
 * Block 0: Superblock (filesystem metadata)
 * Block 1-N: Inode table (file/directory metadata)
 * Block N+1-M: Data blocks (actual file contents)
 * 
 * UNIQUE FEATURES (what makes this YOUR filesystem):
 * 1. Node priority system - files can have priority levels
 * 2. Built-in versioning - each file can have multiple versions
 * 3. Metadata tags - attach custom tags to files
 */

#ifndef XAEFS_H
#define XAEFS_H

#include <stdint.h>

/* Filesystem constants */
#define XAEFS_BLOCK_SIZE 4096       /* 4KB blocks (same as page size) */
#define XAEFS_MAX_FILES 256         /* Max 256 files for now */
#define XAEFS_MAX_FILENAME 64       /* Max filename length */
#define XAEFS_MAX_TAGS 8            /* Max tags per file */
#define XAEFS_TAG_LENGTH 16         /* Max tag string length */

/* File types */
enum xaefs_file_type {
    XAEFS_FILE_REGULAR = 0,
    XAEFS_FILE_DIRECTORY = 1,
    XAEFS_FILE_DEVICE = 2,          /* Device files (future use) */
};

/* File priorities (unique to our FS!) */
enum xaefs_priority {
    XAEFS_PRIORITY_LOW = 0,
    XAEFS_PRIORITY_NORMAL = 1,
    XAEFS_PRIORITY_HIGH = 2,
    XAEFS_PRIORITY_CRITICAL = 3,
};

/*
 * Superblock - Filesystem metadata
 * 
 * WHAT: First block on disk, describes the entire filesystem
 * WHY: We need to know filesystem size, number of files, etc.
 * HOW: Stored at block 0, loaded into memory on mount
 */
struct xaefs_superblock {
    uint32_t magic;                 /* 0x58414546 ("XAEF") - identifies our FS */
    uint32_t version;               /* Filesystem version */
    uint32_t block_size;            /* Size of each block */
    uint32_t total_blocks;          /* Total blocks on disk */
    uint32_t free_blocks;           /* Number of free blocks */
    uint32_t total_inodes;          /* Total inodes (files) */
    uint32_t free_inodes;           /* Number of free inodes */
    char label[32];                 /* Volume label (e.g., "My XAE Disk") */
};

/*
 * Inode - File/Directory metadata
 * 
 * WHAT: Metadata about a single file or directory
 * WHY: We need to track file properties, location, size, etc.
 * HOW: One inode per file, stored in inode table
 * 
 * UNIQUE: We add priority, version, and tags!
 */
struct xaefs_inode {
    char name[XAEFS_MAX_FILENAME];  /* Filename */
    uint32_t inode_num;             /* Inode number (ID) */
    uint32_t parent_inode;          /* Parent directory inode */
    uint32_t size;                  /* File size in bytes */
    uint32_t block_start;           /* First data block */
    uint32_t block_count;           /* Number of blocks used */
    uint8_t type;                   /* File type (regular, dir, etc.) */
    uint8_t priority;               /* File priority (UNIQUE!) */
    uint16_t version;               /* File version number (UNIQUE!) */
    uint32_t created_time;          /* Creation timestamp */
    uint32_t modified_time;         /* Last modification timestamp */
    char tags[XAEFS_MAX_TAGS][XAEFS_TAG_LENGTH];  /* Custom tags (UNIQUE!) */
    uint8_t tag_count;              /* Number of tags */
    uint8_t flags;                  /* Permission/special flags */
};

/*
 * File handle - Open file descriptor
 * 
 * WHAT: Represents an open file
 * WHY: Need to track position, mode, etc. for open files
 * HOW: Created when file is opened, destroyed when closed
 */
struct xaefs_file {
    struct xaefs_inode* inode;      /* Pointer to file's inode */
    uint32_t position;              /* Current read/write position */
    uint8_t mode;                   /* Open mode (read, write, etc.) */
    uint8_t is_open;                /* Is this handle active? */
};

/* Filesystem operations */
void xaefs_init(void);
void xaefs_format(const char* label);

/* File operations */
int xaefs_create(const char* path, uint8_t type, uint8_t priority);
int xaefs_open(const char* path, uint8_t mode);
int xaefs_close(int fd);
int xaefs_read(int fd, void* buffer, uint32_t size);
int xaefs_write(int fd, const void* buffer, uint32_t size);
int xaefs_delete(const char* path);
int xaefs_delete_in_dir(const char* name, const char* current_dir);

/* Directory operations */
int xaefs_mkdir(const char* path, uint8_t priority);
int xaefs_list_dir(const char* path);
int xaefs_set_parent(const char* filename, const char* parent_path);

/* Unique features */
int xaefs_set_priority(const char* path, uint8_t priority);
int xaefs_add_tag(const char* path, const char* tag);
int xaefs_create_version(const char* path);
void xaefs_find_by_tag(const char* tag);

#endif /* XAEFS_H */
