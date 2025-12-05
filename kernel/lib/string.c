/*
 * ==============================================================================
 * STRING IMPLEMENTATION
 * ==============================================================================
 */

#include "include/string.h"

/*
 * strlen() - Get length of string
 * 
 * WHAT: Count characters until null terminator
 * WHY: Need to know string length for many operations
 * HOW: Loop until we find '\0'
 */
size_t strlen(const char* str) 
{
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/*
 * memset() - Fill memory with a value
 * 
 * WHAT: Set all bytes in a memory region to a specific value
 * WHY: Useful for clearing memory or initializing buffers
 * HOW: Loop through and set each byte
 */
void* memset(void* dest, int val, size_t len) 
{
    uint8_t* d = (uint8_t*)dest;
    size_t i;
    
    for (i = 0; i < len; i++) {
        d[i] = (uint8_t)val;
    }
    
    return dest;
}

/*
 * memcpy() - Copy memory from source to destination
 * 
 * WHAT: Copy bytes from one memory location to another
 * WHY: Fundamental operation for moving data around
 * HOW: Copy byte by byte
 */
void* memcpy(void* dest, const void* src, size_t len) 
{
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    size_t i;
    
    for (i = 0; i < len; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

/*
 * memcmp() - Compare two memory regions
 * 
 * WHAT: Compare bytes in two memory locations
 * WHY: To check if memory regions are equal
 * HOW: Compare byte by byte, return difference
 * RETURNS: 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int memcmp(const void* s1, const void* s2, size_t n) 
{
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    size_t i;
    
    for (i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    
    return 0;
}

/*
 * strcmp() - Compare two strings
 * 
 * WHAT: Compare two null-terminated strings
 * WHY: For command parsing and string comparison
 * HOW: Compare character by character
 * RETURNS: 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int strcmp(const char* s1, const char* s2) 
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const uint8_t*)s1 - *(const uint8_t*)s2;
}

/*
 * strcpy() - Copy string
 * 
 * WHAT: Copy source string to destination
 * WHY: For duplicating strings
 * HOW: Copy character by character including null terminator
 */
void strcpy(char* dest, const char* src) 
{
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

/*
 * strtok() - Tokenize string
 * 
 * WHAT: Split string by delimiter
 * WHY: For parsing commands with arguments
 * HOW: Find delimiter, replace with null, return token
 * RETURNS: Pointer to next token, or NULL if no more tokens
 */
static char* strtok_state = NULL;

char* strtok(char* str, char delim) 
{
    char* start;
    
    if (str != NULL) {
        strtok_state = str;
    }
    
    if (strtok_state == NULL || *strtok_state == '\0') {
        return NULL;
    }
    
    /* Skip leading delimiters */
    while (*strtok_state == delim) {
        strtok_state++;
    }
    
    if (*strtok_state == '\0') {
        return NULL;
    }
    
    start = strtok_state;
    
    /* Find next delimiter */
    while (*strtok_state != '\0' && *strtok_state != delim) {
        strtok_state++;
    }
    
    if (*strtok_state != '\0') {
        *strtok_state = '\0';
        strtok_state++;
    }
    
    return start;
}
