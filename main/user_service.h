/*
 * User Service Module - Header
 * Business logic layer for user management operations
 * 
 * SPDX-FileCopyrightText: 2026 Secure Lock Box System
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include "user_store.h"
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Find user by username (case-insensitive)
 * 
 * @param list User list to search
 * @param username Username to find
 * @return int Index of user or -1 if not found
 */
int user_service_find_by_username(const user_list_t *list, const char *username);

/**
 * @brief Find user by user ID
 * 
 * @param list User list to search
 * @param userid User ID to find
 * @return int Index of user or -1 if not found
 */
int user_service_find_by_userid(const user_list_t *list, int userid);

/**
 * @brief Check if username is unique
 * 
 * @param list User list to check
 * @param username Username to check
 * @param ignore_index Index to ignore (-1 for none, used during edit)
 * @return true Username is unique
 * @return false Username already exists
 */
bool user_service_is_unique_username(const user_list_t *list, const char *username, int ignore_index);

/**
 * @brief Get next available user ID
 * 
 * @param list Current user list
 * @return int Next available user ID
 */
int user_service_next_userid(const user_list_t *list);

/**
 * @brief Validate PIN format
 * 
 * @param pin PIN string to validate
 * @return true PIN is valid (4-8 digits)
 * @return false PIN is invalid
 */
bool user_service_validate_pin(const char *pin);

/**
 * @brief Validate username format
 * 
 * @param username Username to validate
 * @return true Username is valid (non-empty, no leading/trailing spaces, <= 32 chars)
 * @return false Username is invalid
 */
bool user_service_validate_username(const char *username);

/**
 * @brief Add new user to list
 * 
 * Auto-assigns user ID. Validates username uniqueness and PIN format.
 * 
 * @param list User list to add to
 * @param new_user User data (userid will be overwritten)
 * @param out_index Optional pointer to receive index of added user
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if validation fails,
 *                   ESP_ERR_NO_MEM if list is full
 */
esp_err_t user_service_add(user_list_t *list, const user_t *new_user, int *out_index);

/**
 * @brief Update existing user
 * 
 * Validates constraints: username uniqueness, cannot demote current admin,
 * FINGERID/FACEID/LAST_LOGON are preserved from existing record.
 * 
 * @param list User list containing user to update
 * @param index Index of user to update
 * @param updated_user Updated user data
 * @param current_admin_userid ID of currently logged in admin
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if validation/constraints fail
 */
esp_err_t user_service_update(user_list_t *list, int index, const user_t *updated_user, 
                               int current_admin_userid);

/**
 * @brief Delete user from list
 * 
 * Prevents deletion of last admin or currently logged in admin.
 * 
 * @param list User list to delete from
 * @param index Index of user to delete
 * @param current_admin_userid ID of currently logged in admin
 * @return esp_err_t ESP_OK on success, ESP_ERR_INVALID_ARG if constraints violated
 */
esp_err_t user_service_delete(user_list_t *list, int index, int current_admin_userid);

/**
 * @brief Count admin users in list
 * 
 * @param list User list to count
 * @return int Number of admin users
 */
int user_service_count_admins(const user_list_t *list);

#ifdef __cplusplus
}
#endif

#endif // USER_SERVICE_H
