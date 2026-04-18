/*
 * User Service Module - Implementation
 * Business logic layer for user management operations
 * 
 * SPDX-FileCopyrightText: 2026 Secure Lock Box System
 * SPDX-License-Identifier: Apache-2.0
 */

#include "user_service.h"
#include "esp_log.h"
#include <string.h>
#include <ctype.h>

static const char *TAG = "USER_SERVICE";

// Helper: case-insensitive string comparison
static int strcasecmp_custom(const char *s1, const char *s2)
{
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

// Find user by username (case-insensitive)
int user_service_find_by_username(const user_list_t *list, const char *username)
{
    if (!list || !list->items || !username) {
        return -1;
    }
    
    for (size_t i = 0; i < list->count; i++) {
        if (strcasecmp_custom(list->items[i].username, username) == 0) {
            return (int)i;
        }
    }
    
    return -1;
}

// Find user by user ID
int user_service_find_by_userid(const user_list_t *list, int userid)
{
    if (!list || !list->items) {
        return -1;
    }
    
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].userid == userid) {
            return (int)i;
        }
    }
    
    return -1;
}

// Check if username is unique
bool user_service_is_unique_username(const user_list_t *list, const char *username, int ignore_index)
{
    if (!list || !list->items || !username) {
        return false;
    }
    
    for (size_t i = 0; i < list->count; i++) {
        if ((int)i == ignore_index) {
            continue; // Skip the user being edited
        }
        
        if (strcasecmp_custom(list->items[i].username, username) == 0) {
            return false; // Username already exists
        }
    }
    
    return true;
}

// Get next available user ID
int user_service_next_userid(const user_list_t *list)
{
    if (!list || !list->items || list->count == 0) {
        return 1;
    }
    
    // Find maximum user ID
    int max_id = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].userid > max_id) {
            max_id = list->items[i].userid;
        }
    }
    
    return max_id + 1;
}

// Validate PIN format (4-8 digits)
bool user_service_validate_pin(const char *pin)
{
    if (!pin) {
        return false;
    }
    
    size_t len = strlen(pin);
    if (len < 4 || len > 8) {
        return false;
    }
    
    for (size_t i = 0; i < len; i++) {
        if (!isdigit((unsigned char)pin[i])) {
            return false;
        }
    }
    
    return true;
}

// Validate username format
bool user_service_validate_username(const char *username)
{
    if (!username) {
        return false;
    }
    
    size_t len = strlen(username);
    
    // Check length
    if (len == 0 || len > USER_STORE_MAX_USERNAME_LEN) {
        return false;
    }
    
    // Check for leading/trailing spaces
    if (isspace((unsigned char)username[0]) || isspace((unsigned char)username[len - 1])) {
        return false;
    }
    
    return true;
}

// Add new user to list
esp_err_t user_service_add(user_list_t *list, const user_t *new_user, int *out_index)
{
    if (!list || !list->items || !new_user) {
        ESP_LOGE(TAG, "Invalid arguments to add user");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check capacity
    if (list->count >= list->capacity) {
        ESP_LOGE(TAG, "User list is full (%zu/%zu)", list->count, list->capacity);
        return ESP_ERR_NO_MEM;
    }
    
    // Validate username
    if (!user_service_validate_username(new_user->username)) {
        ESP_LOGE(TAG, "Invalid username format");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Check username uniqueness
    if (!user_service_is_unique_username(list, new_user->username, -1)) {
        ESP_LOGE(TAG, "Username '%s' already exists", new_user->username);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Validate PIN
    if (!user_service_validate_pin(new_user->pin)) {
        ESP_LOGE(TAG, "Invalid PIN format");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy user data and assign new ID
    user_t *user = &list->items[list->count];
    memcpy(user, new_user, sizeof(user_t));
    user->userid = user_service_next_userid(list);
    
    // Initialize biometric IDs and last logon
    user->fingerid = -1;
    user->faceid = -1;
    user->last_logon[0] = '\0';
    
    list->count++;
    
    if (out_index) {
        *out_index = (int)(list->count - 1);
    }
    
    ESP_LOGI(TAG, "Added user '%s' with ID %d", user->username, user->userid);
    return ESP_OK;
}

// Update existing user
esp_err_t user_service_update(user_list_t *list, int index, const user_t *updated_user, 
                               int current_admin_userid)
{
    if (!list || !list->items || !updated_user) {
        ESP_LOGE(TAG, "Invalid arguments to update user");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (index < 0 || (size_t)index >= list->count) {
        ESP_LOGE(TAG, "Invalid user index %d", index);
        return ESP_ERR_INVALID_ARG;
    }
    
    user_t *user = &list->items[index];
    
    // Validate username if changed
    if (strcmp(user->username, updated_user->username) != 0) {
        if (!user_service_validate_username(updated_user->username)) {
            ESP_LOGE(TAG, "Invalid username format");
            return ESP_ERR_INVALID_ARG;
        }
        
        if (!user_service_is_unique_username(list, updated_user->username, index)) {
            ESP_LOGE(TAG, "Username '%s' already exists", updated_user->username);
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    // Validate PIN if changed
    if (strcmp(user->pin, updated_user->pin) != 0) {
        if (!user_service_validate_pin(updated_user->pin)) {
            ESP_LOGE(TAG, "Invalid PIN format");
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    // Prevent demotion of current admin
    if (user->userid == current_admin_userid && user->admin && !updated_user->admin) {
        ESP_LOGE(TAG, "Cannot demote currently logged in admin");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Update editable fields
    strncpy(user->username, updated_user->username, sizeof(user->username) - 1);
    user->username[sizeof(user->username) - 1] = '\0';
    
    strncpy(user->pin, updated_user->pin, sizeof(user->pin) - 1);
    user->pin[sizeof(user->pin) - 1] = '\0';
    
    user->admin = updated_user->admin;
    
    // FINGERID, FACEID, and LAST_LOGON are NOT updated from UI
    // They are preserved from the existing record
    
    ESP_LOGI(TAG, "Updated user '%s' (ID %d)", user->username, user->userid);
    return ESP_OK;
}

// Delete user from list
esp_err_t user_service_delete(user_list_t *list, int index, int current_admin_userid)
{
    if (!list || !list->items) {
        ESP_LOGE(TAG, "Invalid arguments to delete user");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (index < 0 || (size_t)index >= list->count) {
        ESP_LOGE(TAG, "Invalid user index %d", index);
        return ESP_ERR_INVALID_ARG;
    }
    
    user_t *user = &list->items[index];
    
    // Prevent deletion of current admin
    if (user->userid == current_admin_userid) {
        ESP_LOGE(TAG, "Cannot delete currently logged in admin");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Prevent deletion of last admin
    if (user->admin) {
        int admin_count = user_service_count_admins(list);
        if (admin_count <= 1) {
            ESP_LOGE(TAG, "Cannot delete last admin user");
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    ESP_LOGI(TAG, "Deleting user '%s' (ID %d)", user->username, user->userid);
    
    // Shift remaining users down
    for (size_t i = (size_t)index; i < list->count - 1; i++) {
        memcpy(&list->items[i], &list->items[i + 1], sizeof(user_t));
    }
    
    list->count--;
    
    return ESP_OK;
}

// Count admin users in list
int user_service_count_admins(const user_list_t *list)
{
    if (!list || !list->items) {
        return 0;
    }
    
    int count = 0;
    for (size_t i = 0; i < list->count; i++) {
        if (list->items[i].admin) {
            count++;
        }
    }
    
    return count;
}
