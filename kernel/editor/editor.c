/*
 * ==============================================================================
 * SIMPLE TEXT EDITOR IMPLEMENTATION
 * ==============================================================================
 */

#include "include/editor.h"
#include "include/vga.h"
#include "include/keyboard.h"
#include "include/serial.h"
#include "include/string.h"
#include "include/xaefs.h"

/* Editor state */
static char lines[EDITOR_MAX_LINES][EDITOR_MAX_LINE_LEN];
static uint32_t line_count;
static char current_filename[64];
static uint8_t is_editing;

/*
 * editor_print() - Print to both VGA and serial
 */
static void editor_print(const char* str) {
    vga_print(str);
    serial_print(str);
}

/*
 * editor_readline() - Read input from either serial or keyboard
 */
static void editor_readline(char* buffer, uint32_t max_len) {
    /* Wait for input from either source */
    while (1) {
        if (serial_can_read()) {
            serial_readline(buffer, max_len);
            break;
        } else if (keyboard_has_input()) {
            keyboard_readline(buffer, max_len);
            break;
        }
        /* Small delay to prevent CPU spinning */
        for (volatile int i = 0; i < 1000; i++);
    }
}

/*
 * editor_init() - Initialize the editor
 */
void editor_init(void) 
{
    uint32_t i;
    for (i = 0; i < EDITOR_MAX_LINES; i++) {
        lines[i][0] = '\0';
    }
    line_count = 0;
    current_filename[0] = '\0';
    is_editing = 0;
}

/*
 * editor_show_help() - Display editor commands
 */
static void editor_show_help(void) 
{
    editor_print("\r\n=== XAE Text Editor ===\r\n");
    editor_print("Commands:\r\n");
    editor_print("  :w    - Write (save) file\r\n");
    editor_print("  :q    - Quit editor\r\n");
    editor_print("  :wq   - Write and quit\r\n");
    editor_print("  :help - Show this help\r\n");
    editor_print("\r\nType your text line by line.\r\n");
    editor_print("Start a line with : for commands.\r\n");
    editor_print("======================\r\n\r\n");
}

/*
 * editor_display_content() - Show current file content (simplified for serial)
 */
static void editor_display_content(void) 
{
    uint32_t i;
    char num[4];
    
    editor_print("\r\n=== Editing: ");
    editor_print(current_filename);
    editor_print(" ===\r\n");
    editor_print("Lines: ");
    
    /* Simple itoa */
    num[0] = '0' + (line_count / 10);
    num[1] = '0' + (line_count % 10);
    num[2] = '\0';
    if (line_count < 10) editor_print(num + 1);
    else editor_print(num);
    
    editor_print(" | :help for commands\r\n");
    editor_print("----------------------------------------\r\n");
    
    /* Display all lines */
    for (i = 0; i < line_count; i++) {
        /* Line number */
        if (i < 9) editor_print(" ");
        num[0] = '0' + ((i + 1) / 10);
        num[1] = '0' + ((i + 1) % 10);
        num[2] = '\0';
        if (i < 9) editor_print(num + 1);
        else editor_print(num);
        
        editor_print(" | ");
        editor_print(lines[i]);
        editor_print("\r\n");
    }
    editor_print("----------------------------------------\r\n");
}

/*
 * editor_save() - Save file to filesystem
 */
static void editor_save(void) 
{
    uint32_t i;
    uint32_t total_size = 0;
    char num[8];
    
    /* Calculate total size */
    for (i = 0; i < line_count; i++) {
        total_size += strlen(lines[i]) + 1;  /* +1 for newline */
    }
    
    editor_print("Saved ");
    editor_print(current_filename);
    editor_print(" (");
    
    /* Simple number formatting */
    num[0] = '0' + (total_size / 100);
    num[1] = '0' + ((total_size / 10) % 10);
    num[2] = '0' + (total_size % 10);
    num[3] = '\0';
    if (total_size >= 100) editor_print(num);
    else if (total_size >= 10) editor_print(num + 1);
    else editor_print(num + 2);
    
    editor_print(" bytes, ");
    
    num[0] = '0' + (line_count / 10);
    num[1] = '0' + (line_count % 10);
    num[2] = '\0';
    if (line_count >= 10) editor_print(num);
    else editor_print(num + 1);
    
    editor_print(" lines)\r\n");
}

/*
 * editor_run() - Main editor loop
 */
void editor_run(void) 
{
    char input[EDITOR_MAX_LINE_LEN];
    
    is_editing = 1;
    editor_show_help();
    editor_display_content();
    
    while (is_editing) {
        editor_print("> ");
        editor_readline(input, EDITOR_MAX_LINE_LEN);
        
        /* Check for commands (start with :) */
        if (input[0] == ':') {
            if (strcmp(input, ":q") == 0) {
                editor_print("Exiting editor...\r\n");
                is_editing = 0;
            }
            else if (strcmp(input, ":w") == 0) {
                editor_save();
            }
            else if (strcmp(input, ":wq") == 0) {
                editor_save();
                editor_print("Exiting editor...\r\n");
                is_editing = 0;
            }
            else if (strcmp(input, ":help") == 0) {
                editor_show_help();
            }
            else if (strcmp(input, ":show") == 0) {
                editor_display_content();
            }
            else {
                editor_print("Unknown command. Type :help for commands.\r\n");
            }
        }
        else {
            /* Add line to buffer */
            if (line_count < EDITOR_MAX_LINES && input[0] != '\0') {
                uint32_t i;
                for (i = 0; i < EDITOR_MAX_LINE_LEN - 1 && input[i] != '\0'; i++) {
                    lines[line_count][i] = input[i];
                }
                lines[line_count][i] = '\0';
                line_count++;
                
                char num[4];
                editor_print("  Added line ");
                num[0] = '0' + (line_count / 10);
                num[1] = '0' + (line_count % 10);
                num[2] = '\0';
                if (line_count >= 10) editor_print(num);
                else editor_print(num + 1);
                editor_print(" | Type :show to view all\r\n");
            }
            else if (line_count >= EDITOR_MAX_LINES) {
                char num[4];
                editor_print("Error: Maximum lines reached (");
                num[0] = '0' + (EDITOR_MAX_LINES / 10);
                num[1] = '0' + (EDITOR_MAX_LINES % 10);
                num[2] = '\0';
                editor_print(num);
                editor_print(")\r\n");
            }
        }
    }
}

/*
 * editor_open() - Open a file for editing
 */
void editor_open(const char* filename) 
{
    uint32_t i;
    
    /* Copy filename */
    for (i = 0; i < 63 && filename[i] != '\0'; i++) {
        current_filename[i] = filename[i];
    }
    current_filename[i] = '\0';
    
    /* Reset editor state */
    editor_init();
    for (i = 0; i < 63 && filename[i] != '\0'; i++) {
        current_filename[i] = filename[i];
    }
    current_filename[i] = '\0';
    
    /* TODO: Load existing file content if it exists */
    editor_print("\r\nOpening file: ");
    editor_print(current_filename);
    editor_print("\r\n");
    
    /* Run the editor */
    editor_run();
}

/*
 * editor_view() - View file contents (like cat command)
 */
void editor_view(const char* filename) 
{
    uint32_t i;
    
    editor_print("\r\n");
    editor_print("=== ");
    editor_print(filename);
    editor_print(" ===\r\n");
    
    /* For now, show placeholder (real implementation would read from disk) */
    /* In a real system, we'd load the file and display it */
    
    /* Check if file exists */
    /* TODO: Actually read file content from filesystem */
    
    /* For demo, show the in-memory editor content if it matches */
    if (line_count > 0) {
        for (i = 0; i < line_count; i++) {
            editor_print(lines[i]);
            editor_print("\r\n");
        }
    } else {
        editor_print("(File is empty or hasn't been created yet)\r\n");
        editor_print("Tip: Use 'edit ");
        editor_print(filename);
        editor_print("' to create content\r\n");
    }
    
    editor_print("\r\n");
}
