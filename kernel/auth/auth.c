#include "include/auth.h"
#include "include/string.h"

static user_t users[MAX_USERS];
static uint8_t num_users = 0;

void auth_init(void) {
    // Clear all users
    for (int i = 0; i < MAX_USERS; i++) {
        users[i].active = 0;
    }
    
    // Add default admin user
    auth_add_user("admin", "admin123");
    auth_add_user("user", "password");
}

void auth_hash_password(const char* password, char* hash_out) {
    // Simple hash: XOR each char with position and magic number
    // NOT cryptographically secure, but simple for this use case
    uint8_t len = strlen(password);
    for (uint8_t i = 0; i < len && i < 63; i++) {
        hash_out[i] = password[i] ^ (i + 0x42);
    }
    hash_out[len] = 0;
}

void auth_add_user(const char* username, const char* password) {
    if (num_users >= MAX_USERS) return;
    
    strcpy(users[num_users].username, username);
    auth_hash_password(password, users[num_users].password);
    users[num_users].active = 1;
    num_users++;
}

uint8_t auth_verify(const char* username, const char* password) {
    char hash[64];
    auth_hash_password(password, hash);
    
    for (uint8_t i = 0; i < MAX_USERS; i++) {
        if (users[i].active && 
            strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].password, hash) == 0) {
            return 1;
        }
    }
    return 0;
}

void encrypt_data(uint8_t* data, uint16_t length, uint8_t key) {
    for (uint16_t i = 0; i < length; i++) {
        data[i] ^= key;
    }
}

void decrypt_data(uint8_t* data, uint16_t length, uint8_t key) {
    // XOR encryption is symmetric
    encrypt_data(data, length, key);
}
