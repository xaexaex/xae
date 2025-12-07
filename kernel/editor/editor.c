/*
 * ==============================================================================
 * SIMPLE TEXT EDITOR IMPLEMENTATION
 * ==============================================================================
 */

#include "include/editor.h"
#include "include/vga.h"
#include "include/keyboard.h"
#include "include/string.h"
#include "include/xaefs.h"

/* Editor state */
static char lines[EDITOR_MAX_LINES][EDITOR_MAX_LINE_LEN];
static uint32_t line_count;
static char current_filename[64];
static uint8_t is_editing;

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
    vga_print("\n=== XAE Text Editor ===\n");
    vga_print("Commands:\n");
    vga_print("  :w    - Write (save) file\n");
    vga_print("  :q    - Quit editor\n");
    vga_print("  :wq   - Write and quit\n");
    vga_print("  :help - Show this help\n");
    vga_print("\nType your text line by line.\n");
    vga_print("Start a line with : for commands.\n");
    vga_print("======================\n\n");
}

/*
 * editor_display_content() - Show current file content
 */
static void editor_display_content(void) 
{
    uint32_t i;
    
    vga_clear();
    vga_print("=== Editing: ");
    vga_print(current_filename);
    vga_print(" ===\n");
    vga_print("Lines: ");
    
    /* Show line count */
    if (line_count == 0) {
        vga_print("0");
    } else {
        /* Simple number print */
        if (line_count >= 10) vga_putchar('0' + (line_count / 10));
        vga_putchar('0' + (line_count % 10));
    }
    vga_print(" | :help for commands\n");
    vga_print("----------------------------------------\n");
    
    /* Display all lines */
    for (i = 0; i < line_count; i++) {
        /* Line number */
        if (i < 9) vga_putchar(' ');
        vga_putchar('0' + ((i + 1) / 10));
        vga_putchar('0' + ((i + 1) % 10));
        vga_print(" | ");
        vga_print(lines[i]);
        vga_putchar('\n');
    }
    vga_print("----------------------------------------\n");
}

/*
 * editor_save() - Save file to filesystem
 */
static void editor_save(void) 
{
    uint32_t i, j;
    uint32_t total_size = 0;
    
    /* Calculate total size */
    for (i = 0; i < line_count; i++) {
        total_size += strlen(lines[i]) + 1;  /* +1 for newline */
    }
    
    /* For now, just confirm save (real implementation would write to disk) */
    vga_print("Saved ");
    vga_print(current_filename);
    vga_print(" (");
    
    /* Print size */
    if (total_size >= 100) vga_putchar('0' + (total_size / 100));
    if (total_size >= 10) vga_putchar('0' + ((total_size / 10) % 10));
    vga_putchar('0' + (total_size % 10));
    
    vga_print(" bytes, ");
    if (line_count >= 10) vga_putchar('0' + (line_count / 10));
    vga_putchar('0' + (line_count % 10));
    vga_print(" lines)\n");
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
        vga_print("> ");
        keyboard_readline(input, EDITOR_MAX_LINE_LEN);
        
        /* Check for commands (start with :) */
        if (input[0] == ':') {
            if (strcmp(input, ":q") == 0) {
                vga_print("Exiting editor...\n");
                is_editing = 0;
            }
            else if (strcmp(input, ":w") == 0) {
                editor_save();
            }
            else if (strcmp(input, ":wq") == 0) {
                editor_save();
                vga_print("Exiting editor...\n");
                is_editing = 0;
            }
            else if (strcmp(input, ":help") == 0) {
                editor_show_help();
            }
            else if (strcmp(input, ":show") == 0) {
                editor_display_content();
            }
            else {
                vga_print("Unknown command. Type :help for commands.\n");
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
                
                vga_print("  Added line ");
                if (line_count >= 10) vga_putchar('0' + (line_count / 10));
                vga_putchar('0' + (line_count % 10));
                vga_print(" | Type :show to view all\n");
            }
            else if (line_count >= EDITOR_MAX_LINES) {
                vga_print("Error: Maximum lines reached (");
                vga_putchar('0' + (EDITOR_MAX_LINES / 10));
                vga_putchar('0' + (EDITOR_MAX_LINES % 10));
                vga_print(")\n");
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
    vga_print("\nOpening file: ");
    vga_print(current_filename);
    vga_print("\n");
    
    /* Run the editor */
    editor_run();
}

/*
 * editor_view() - View file contents (like cat command)
 */
void editor_view(const char* filename) 
{
    uint32_t i;
    
    vga_print("\n");
    vga_print("=== ");
    vga_print(filename);
    vga_print(" ===\n");
    
    /* For now, show placeholder (real implementation would read from disk) */
    /* In a real system, we'd load the file and display it */
    
    /* Check if file exists */
    /* TODO: Actually read file content from filesystem */
    
    /* For demo, show the in-memory editor content if it matches */
    if (line_count > 0) {
        for (i = 0; i < line_count; i++) {
            vga_print(lines[i]);
            vga_putchar('\n');
        }
    } else {
        vga_print("(File is empty or hasn't been created yet)\n");
        vga_print("Tip: Use 'edit ");
        vga_print(filename);
        vga_print("' to create content\n");
    }
    
    vga_print("\n");
}
