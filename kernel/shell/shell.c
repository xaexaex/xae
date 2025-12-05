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

#define CMD_BUFFER_SIZE 256

/* Command buffer */
static char cmd_buffer[CMD_BUFFER_SIZE];

/*
 * shell_init() - Initialize the shell
 */
void shell_init(void) 
{
    vga_print("\n");
    vga_print("XAE Shell v1.0\n");
    vga_print("Type 'help' for available commands\n");
    vga_print("\n");
}

/*
 * cmd_help() - Show available commands
 */
static void cmd_help(void) 
{
    vga_print("\nAvailable Commands:\n");
    vga_print("  mk <name>         - Create file or folder\n");
    vga_print("  rm <name>         - Remove file\n");
    vga_print("  ls                - List all files\n");
    vga_print("  tag <file> <tag>  - Add tag to file\n");
    vga_print("  find <tag>        - Find files by tag\n");
    vga_print("  pri <file> <lvl>  - Set priority (low/mid/high/max)\n");
    vga_print("  ver <file>        - Create new version\n");
    vga_print("  back <file> <num> - Restore to version\n");
    vga_print("  info <file>       - Show file details\n");
    vga_print("  clear             - Clear screen\n");
    vga_print("  help              - Show this help\n");
    vga_print("\n");
}

/*
 * cmd_mk() - Create file or directory
 */
static void cmd_mk(char* name) 
{
    if (!name) {
        vga_print("Usage: mk <name>\n");
        return;
    }
    
    int result = xaefs_create(name, XAEFS_FILE_REGULAR, XAEFS_PRIORITY_NORMAL);
    if (result >= 0) {
        vga_print("Created: ");
        vga_print(name);
        vga_print("\n");
    } else {
        vga_print("Error: Could not create file\n");
    }
}

/*
 * cmd_ls() - List files
 */
static void cmd_ls(void) 
{
    xaefs_list_dir("/");
}

/*
 * cmd_tag() - Add tag to file
 */
static void cmd_tag(char* file, char* tag) 
{
    if (!file || !tag) {
        vga_print("Usage: tag <file> <tag>\n");
        return;
    }
    
    int result = xaefs_add_tag(file, tag);
    if (result == 0) {
        vga_print("Tagged '");
        vga_print(file);
        vga_print("' with '");
        vga_print(tag);
        vga_print("'\n");
    } else {
        vga_print("Error: Could not add tag\n");
    }
}

/*
 * cmd_find() - Find files by tag
 */
static void cmd_find(char* tag) 
{
    if (!tag) {
        vga_print("Usage: find <tag>\n");
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
        vga_print("Usage: pri <file> <level>\n");
        vga_print("Levels: low, mid, high, max\n");
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
        vga_print("Invalid level. Use: low, mid, high, max\n");
        return;
    }
    
    int result = xaefs_set_priority(file, priority);
    if (result == 0) {
        vga_print("Priority set to ");
        vga_print(level);
        vga_print("\n");
    } else {
        vga_print("Error: File not found\n");
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
    else if (strcmp(token, "rm") == 0) {
        vga_print("Command 'rm' not yet implemented\n");
    }
    else if (strcmp(token, "ver") == 0) {
        vga_print("Command 'ver' not yet implemented\n");
    }
    else if (strcmp(token, "back") == 0) {
        vga_print("Command 'back' not yet implemented\n");
    }
    else if (strcmp(token, "info") == 0) {
        vga_print("Command 'info' not yet implemented\n");
    }
    else {
        vga_print("Unknown command: ");
        vga_print(token);
        vga_print("\nType 'help' for available commands\n");
    }
}

/*
 * shell_run() - Main shell loop
 * 
 * WHAT: Interactive command loop
 * WHY: To continuously accept and execute commands
 * HOW: Show prompt, read input, parse, execute, repeat
 */
void shell_run(void) 
{
    while (1) {
        /* Show prompt */
        vga_print("> ");
        
        /* Read command */
        keyboard_readline(cmd_buffer, CMD_BUFFER_SIZE);
        
        /* Parse and execute */
        if (cmd_buffer[0] != '\0') {
            parse_and_execute(cmd_buffer);
        }
    }
}
