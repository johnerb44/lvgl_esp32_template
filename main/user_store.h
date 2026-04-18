/*
 * User Store Module - Header
 * Data persistence layer for user management with JSON storage
 * 
 * SPDX-FileCopyrightText: 2026 Secure Lock Box System
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USER_STORE_H
#define USER_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define USER_STORE_MAX_USERS 64
#define USER_STORE_MAX_USERNAME_LEN 32
#define USER_STORE_MAX_PIN_LEN 8
#define USER_STORE_MAX_TIMESTAMP_LEN 32
#define USER_STORE_DEFAULT_PATH "/sdcard/users.jsn"

// User data structure matching JSON schema
typedef struct {
    int userid;                                      // Unique user ID
    int fingerid;                                    // Fingerprint ID (-1 if not enrolled)
    int faceid;                                      // Face ID (-1 if not enrolled)
    char pin[USER_STORE_MAX_PIN_LEN + 1];           // PIN code (4-8 digits as string)
    char username[USER_STORE_MAX_USERNAME_LEN + 1]; // Display name
    bool admin;                                      // Admin flag
    char last_logon[USER_STORE_MAX_TIMESTAMP_LEN + 1]; // ISO 8601 timestamp or empty
} user_t;

// User list container
typedef struct {
    user_t *items;      // Array of users
    size_t count;       // Current number of users
    size_t capacity;    // Maximum capacity
} user_list_t;

/**
 * @brief Initialize the user store module
 * 
 * @param path File path for JSON storage (NULL for default)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t user_store_init(const char *path);

/**
 * @brief Load users from JSON file
 * 
 * Creates default admin user if file doesn't exist.
 * Handles corrupted JSON by backing up and creating new default.
 * 
 * @param out_list Pointer to user_list_t to populate
 * @return esp_err_t ESP_OK on success
 */
esp_err_t user_store_load(user_list_t *out_list);

/**
 * @brief Save users to JSON file
 * 
 * Uses atomic write (temp file + rename) for safety.
 * 
 * @param list User list to save
 * @return esp_err_t ESP_OK on success
 */
esp_err_t user_store_save(const user_list_t *list);

/**
 * @brief Free user list memory
 * 
 * @param list User list to free
 */
void user_store_free(user_list_t *list);

/**
 * @brief Get current file path
 * 
 * @return const char* Path to JSON file
 */
const char* user_store_get_path(void);

#ifdef __cplusplus
}
#endif

#endif // USER_STORE_H
