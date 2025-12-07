/*
 * ==============================================================================
 * SIMPLE TEXT EDITOR
 * ==============================================================================
 * WHAT: A basic line-based text editor
 * WHY: To create and edit files in the OS
 * HOW: Line-by-line editing with save/exit commands
 */

#ifndef EDITOR_H
#define EDITOR_H

#include <stdint.h>

/* Editor configuration */
#define EDITOR_MAX_LINES 50
#define EDITOR_MAX_LINE_LEN 80

/* Editor functions */
void editor_init(void);
void editor_open(const char* filename);
void editor_run(void);
void editor_view(const char* filename);  /* View file contents */

#endif /* EDITOR_H */
