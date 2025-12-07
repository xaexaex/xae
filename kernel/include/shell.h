/*
 * ==============================================================================
 * XAE SHELL - Interactive Command Line Interface
 * ==============================================================================
 * WHAT: Command parser and executor for XAE OS
 * WHY: To allow users to interact with the filesystem and OS
 * HOW: Read commands, parse arguments, execute functions
 */

#ifndef SHELL_H
#define SHELL_H

/* Initialize and run the shell */
void shell_init(void);
void shell_run(void);

#endif /* SHELL_H */
