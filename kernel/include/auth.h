#ifndef AUTH_H
#define AUTH_H

#include <stdint.h>

// User credentials
#define MAX_USERS 5

typedef struct {
    char username[32];
    char password[64];  // Hashed password
    uint8_t active;
} user_t;

// Authentication functions
void auth_init(void);
void auth_add_user(const char* username, const char* password);
uint8_t auth_verify(const char* username, const char* password);
void auth_hash_password(const char* password, char* hash_out);

// Simple XOR encryption
void encrypt_data(uint8_t* data, uint16_t length, uint8_t key);
void decrypt_data(uint8_t* data, uint16_t length, uint8_t key);

#endif
