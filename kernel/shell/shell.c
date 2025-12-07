/*
 * ==============================================================================
 * XAE SHELL IMPLEMENTATION
 * ==============================================================================
 */

#include "include/shell.h"
#include "include/vga.h"
#include "include/keyboard.h"
#include "include/string.h"
#include "include/xaefs.h"
#include "include/editor.h"
#include "include/serial.h"

#define CMD_BUFFER_SIZE 256
#define PATH_BUFFER_SIZE 128

/* Command buffer */
static char cmd_buffer[CMD_BUFFER_SIZE];

/* Current directory path */
static char current_path[PATH_BUFFER_SIZE] = "/";

/* Dual output helper */
static void shell_print(const char* str) {
    vga_print(str);
    serial_print(str);
}

/* Network output helper */
static void shell_net_print(void* session, const char* str) {
    extern void net_send_tcp(void* session, const char* data, uint16_t length);
    if (session) {
        net_send_tcp(session, str, (uint16_t)strlen(str));
    }
}

/*
 * shell_init() - Initialize the shell
 */
void shell_init(void) 
{
    shell_print("\n");
}

/*
 * cmd_help() - Show available commands
 */
static void cmd_help(void) 
{
    shell_print("\nAvailable Commands (Part 1/2):\n");
    shell_print("  mk <name>         - Create file\n");
    shell_print("  mk <name>/        - Create folder\n");
    shell_print("  cd <dir>          - Change directory\n");
    shell_print("  rm <name>         - Remove file\n");
    shell_print("  ls                - List files\n");
    shell_print("  edit <file>       - Text editor\n");
    shell_print("  fun <file>        - View file\n");
    shell_print("  sync              - Save to disk\n");
    shell_print("\nPart 2/2:\n");
    shell_print("  tag <file> <tag>  - Add tag\n");
    shell_print("  find <tag>        - Find by tag\n");
    shell_print("  pri <file> <lvl>  - Set priority\n");
    shell_print("                      (low/mid/high/max)\n");
    shell_print("  clear             - Clear screen\n");
    shell_print("  help              - This help\n");
    shell_print("\n");
}

/*
 * cmd_mk() - Create file or directory
 */
static void cmd_mk(char* name) 
{
    if (!name) {
        shell_print("Usage: mk <name>     (creates file)\n");
        shell_print("   or: mk <name>/    (creates folder - add / at end)\n");
        return;
    }
    
    /* Check if name ends with / for directory */
    uint32_t len = 0;
    while (name[len] != '\0') len++;
    
    uint8_t is_dir = 0;
    if (len > 0 && name[len - 1] == '/') {
        is_dir = 1;
        name[len - 1] = '\0';  /* Remove the / */
    }
    
    /* Build full path: current_path + name */
    char full_path[PATH_BUFFER_SIZE];
    int i = 0, j = 0;
    
    /* Copy current path */
    while (current_path[i] != '\0' && i < PATH_BUFFER_SIZE - 1) {
        full_path[j++] = current_path[i++];
    }
    
    /* Add / if not at root and path doesn't end with / */
    if (j > 0 && full_path[j-1] != '/') {
        full_path[j++] = '/';
    }
    
    /* Add filename */
    i = 0;
    while (name[i] != '\0' && j < PATH_BUFFER_SIZE - 1) {
        full_path[j++] = name[i++];
    }
    full_path[j] = '\0';
    
    uint8_t type = is_dir ? XAEFS_FILE_DIRECTORY : XAEFS_FILE_REGULAR;
    int result = xaefs_create(full_path, type, XAEFS_PRIORITY_NORMAL);
    
    if (result >= 0) {
        /* Set parent directory */
        xaefs_set_parent(full_path, current_path);
        
        /* Manually sync to ensure parent is saved */
        xaefs_sync();
        
        shell_print("Created ");
        shell_print(is_dir ? "folder: " : "file: ");
        shell_print(name);
        shell_print(" in ");
        shell_print(current_path);
        shell_print("\n");
    } else {
        if (result == -2) {
            shell_print("Error: File system is full\n");
        } else if (result == -3) {
            shell_print("Error: File already exists: ");
            shell_print(name);
            shell_print("\n");
        } else {
            shell_print("Error: Could not create ");
            shell_print(is_dir ? "folder\n" : "file\n");
        }
    }
}

/*
 * cmd_ls() - List files in current directory
 */
static void cmd_ls(void) 
{
    xaefs_list_dir(current_path);
}

/*
 * cmd_rm() - Remove file or directory
 */
static void cmd_rm(char* name) 
{
    if (!name) {
        shell_print("Usage: rm <name>\n");
        return;
    }
    
    int result = xaefs_delete_in_dir(name, current_path);
    
    if (result == 0) {
        shell_print("Deleted: ");
        shell_print(name);
        shell_print("\n");
    } else {
        shell_print("Error: File not found or cannot be deleted: ");
        shell_print(name);
        shell_print("\n");
    }
}

/*
 * cmd_cd() - Change directory
 */
static void cmd_cd(char* dirname) 
{
    if (!dirname) {
        shell_print("Usage: cd <directory>\n");
        shell_print("   or: cd ..  (go up one level)\n");
        shell_print("   or: cd /   (go to root)\n");
        return;
    }
    
    /* Handle special cases */
    if (strcmp(dirname, "/") == 0) {
        current_path[0] = '/';
        current_path[1] = '\0';
        shell_print("Changed to: /\n");
        return;
    }
    
    if (strcmp(dirname, "..") == 0) {
        /* Go up one level */
        if (strcmp(current_path, "/") == 0) {
            shell_print("Already at root directory\n");
            return;
        }
        
        /* Find last / and truncate */
        int i = 0;
        while (current_path[i] != '\0') i++;
        i--;
        while (i > 0 && current_path[i] != '/') i--;
        
        if (i == 0) {
            /* Going back to root */
            current_path[0] = '/';
            current_path[1] = '\0';
        } else {
            current_path[i] = '\0';
        }
        shell_print("Changed to: ");
        shell_print(current_path);
        shell_print("\n");
        return;
    }
    
    /* Check if directory exists (simplified - assumes it exists) */
    /* Build new path */
    char new_path[PATH_BUFFER_SIZE];
    int i = 0, j = 0;
    
    /* Copy current path */
    while (current_path[i] != '\0' && i < PATH_BUFFER_SIZE - 1) {
        new_path[j++] = current_path[i++];
    }
    
    /* Add / if needed */
    if (j > 0 && new_path[j-1] != '/') {
        new_path[j++] = '/';
    }
    
    /* Add dirname */
    i = 0;
    while (dirname[i] != '\0' && j < PATH_BUFFER_SIZE - 1) {
        new_path[j++] = dirname[i++];
    }
    new_path[j] = '\0';
    
    /* Update current path */
    i = 0;
    while (new_path[i] != '\0' && i < PATH_BUFFER_SIZE - 1) {
        current_path[i] = new_path[i];
        i++;
    }
    current_path[i] = '\0';
    
    shell_print("Changed to: ");
    shell_print(current_path);
    shell_print("\n");
}

/*
 * cmd_tag() - Add tag to file
 */
static void cmd_tag(char* file, char* tag) 
{
    if (!file || !tag) {
        shell_print("Usage: tag <file> <tag>\n");
        return;
    }
    
    int result = xaefs_add_tag(file, tag);
    if (result == 0) {
        shell_print("Tagged '");
        shell_print(file);
        shell_print("' with '");
        shell_print(tag);
        shell_print("'\n");
    } else {
        shell_print("Error: Could not add tag\n");
    }
}

/*
 * cmd_edit() - Open text editor
 */
static void cmd_edit(char* filename) 
{
    if (!filename) {
        shell_print("Usage: edit <filename>\n");
        return;
    }
    
    editor_open(filename);
}

/*
 * cmd_fun() - View file contents (like cat)
 */
static void cmd_fun(char* filename) 
{
    if (!filename) {
        shell_print("Usage: fun <filename>\n");
        return;
    }
    
    editor_view(filename);
}

/*
 * cmd_find() - Find files by tag
 */
static void cmd_find(char* tag) 
{
    if (!tag) {
        shell_print("Usage: find <tag>\n");
        return;
    }
    
    xaefs_find_by_tag(tag);
}

/*
 * cmd_pri() - Set file priority
 */
static void cmd_pri(char* file, char* level) 
{
    if (!file || !level) {
        shell_print("Usage: pri <file> <level>\n");
        shell_print("Levels: low, mid, high, max\n");
        return;
    }
    
    uint8_t priority;
    if (strcmp(level, "low") == 0) {
        priority = XAEFS_PRIORITY_LOW;
    } else if (strcmp(level, "mid") == 0) {
        priority = XAEFS_PRIORITY_NORMAL;
    } else if (strcmp(level, "high") == 0) {
        priority = XAEFS_PRIORITY_HIGH;
    } else if (strcmp(level, "max") == 0) {
        priority = XAEFS_PRIORITY_CRITICAL;
    } else {
        shell_print("Invalid level. Use: low, mid, high, max\n");
        return;
    }
    
    int result = xaefs_set_priority(file, priority);
    if (result == 0) {
        shell_print("Priority set to ");
        shell_print(level);
        shell_print("\n");
    } else {
        shell_print("Error: File not found\n");
    }
}

/*
 * cmd_clear() - Clear screen
 */
static void cmd_clear(void) 
{
    vga_clear();
}

/*
 * cmd_sync() - Save filesystem to disk
 */
static void cmd_sync(void) 
{
    shell_print("Manually syncing filesystem to disk...\n");
    xaefs_sync();
    shell_print("[OK] Filesystem synced successfully\n");
}

/*
 * cmd_debug() - Show all inodes for debugging
 */
static void cmd_debug(void) 
{
    xaefs_debug_list_all();
}

/*
 * parse_and_execute() - Parse command and execute
 */
static void parse_and_execute(char* cmd) 
{
    char* token;
    char* arg1 = NULL;
    char* arg2 = NULL;
    
    /* Get command */
    token = strtok(cmd, ' ');
    if (!token) return;
    
    /* Get arguments */
    arg1 = strtok(NULL, ' ');
    arg2 = strtok(NULL, ' ');
    
    /* Execute command */
    if (strcmp(token, "help") == 0) {
        cmd_help();
    }
    else if (strcmp(token, "mk") == 0) {
        cmd_mk(arg1);
    }
    else if (strcmp(token, "ls") == 0) {
        cmd_ls();
    }
    else if (strcmp(token, "tag") == 0) {
        cmd_tag(arg1, arg2);
    }
    else if (strcmp(token, "find") == 0) {
        cmd_find(arg1);
    }
    else if (strcmp(token, "pri") == 0) {
        cmd_pri(arg1, arg2);
    }
    else if (strcmp(token, "clear") == 0) {
        cmd_clear();
    }
    else if (strcmp(token, "cd") == 0) {
        cmd_cd(arg1);
    }
    else if (strcmp(token, "rm") == 0) {
        cmd_rm(arg1);
    }
    else if (strcmp(token, "edit") == 0) {
        cmd_edit(arg1);
    }
    else if (strcmp(token, "fun") == 0) {
        cmd_fun(arg1);
    }
    else if (strcmp(token, "sync") == 0) {
        cmd_sync();
    }
    else if (strcmp(token, "debug") == 0) {
        cmd_debug();
    }
    else if (strcmp(token, "ver") == 0) {
        shell_print("Command 'ver' not yet implemented\n");
    }
    else if (strcmp(token, "back") == 0) {
        shell_print("Command 'back' not yet implemented\n");
    }
    else if (strcmp(token, "info") == 0) {
        shell_print("Command 'info' not yet implemented\n");
    }
    else {
        shell_print("Unknown command: ");
        shell_print(token);
        shell_print("\nType 'help' for available commands\n");
    }
}

/*
 * shell_run() - Main shell loop
 */
void shell_run(void) 
{
    while (1) {
        /* Show prompt */
        shell_print(current_path);
        shell_print(" > ");
        
        /* Wait for input - poll network while waiting */
        extern void rtl8139_handle_interrupt(void);
        
        while (1) {
            /* Check for network packets */
            rtl8139_handle_interrupt();
            
            /* Check for input */
            if (serial_can_read()) {
                serial_readline(cmd_buffer, CMD_BUFFER_SIZE);
                break;
            } else if (keyboard_has_input()) {
                keyboard_readline(cmd_buffer, CMD_BUFFER_SIZE);
                break;
            }
            
            /* Small delay to prevent CPU spinning */
            for (volatile int i = 0; i < 1000; i++);
        }
        
        /* Parse and execute */
        if (cmd_buffer[0] != '\0') {
            parse_and_execute(cmd_buffer);
        }
    }
}

/*
 * shell_execute_command() - Execute command from network session
 */
void shell_execute_command(const char* cmd, void* session) {
    char buffer[CMD_BUFFER_SIZE];
    
    /* Copy command to buffer */
    uint16_t i = 0;
    while (cmd[i] != '\0' && cmd[i] != '\n' && cmd[i] != '\r' && i < CMD_BUFFER_SIZE - 1) {
        buffer[i] = cmd[i];
        i++;
    }
    buffer[i] = '\0';
    
    /* Temporarily redirect output to network session */
    /* For now, just acknowledge the command */
    shell_net_print(session, "\nExecuting: ");
    shell_net_print(session, buffer);
    shell_net_print(session, "\n");
    
    /* Parse command (reuse existing logic) */
    char* token = buffer;
    while (*token == ' ') token++;
    
    if (*token == '\0') {
        shell_net_print(session, "> ");
        return;
    }
    
    /* Execute based on command */
    if (strncmp(token, "ls", 2) == 0) {
        /* List files and send to network */
        shell_net_print(session, "Files in current directory:\n");
        /* TODO: Implement ls output to network */
    } else if (strncmp(token, "help", 4) == 0) {
        shell_net_print(session, "XAE Shell Commands:\n");
        shell_net_print(session, "  ls, cd, mk, rm, edit, fun, sync, help\n");
    } else {
        shell_net_print(session, "Command not yet supported via network\n");
    }
    
    shell_net_print(session, "> ");
}

