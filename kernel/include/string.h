/*
 * ==============================================================================
 * STRING UTILITIES
 * ==============================================================================
 * WHAT: Basic string manipulation functions
 * WHY: We can't use standard library (libc) in kernel mode
 * HOW: Implement our own versions of common string functions
 */

#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include <stddef.h>

/* String functions */
size_t strlen(const char* str);
void* memset(void* dest, int val, size_t len);
void* memcpy(void* dest, const void* src, size_t len);
int memcmp(const void* s1, const void* s2, size_t n);

#endif /* STRING_H */
