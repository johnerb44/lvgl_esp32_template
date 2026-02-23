/*
 * User Store Module - Implementation
 * Data persistence layer for user management with JSON storage
 * 
 * SPDX-FileCopyrightText: 2026 Secure Lock Box System
 * SPDX-License-Identifier: Apache-2.0
 */

#include "user_store.h"
#include "cJSON/cJSON.h"
#include "esp_log.h"
#include "ch422g_driver.h"
#include "sd/sd_card.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

static const char *TAG = "USER_STORE";
static char s_file_path[128] = USER_STORE_DEFAULT_PATH;

// Helper function to create default admin user
static void create_default_admin(user_t *user)
{
    user->userid = 1;
    user->fingerid = -1;
    user->faceid = -1;
    strncpy(user->pin, "1234", sizeof(user->pin) - 1);
    user->pin[sizeof(user->pin) - 1] = '\0';
    strncpy(user->username, "Admin", sizeof(user->username) - 1);
    user->username[sizeof(user->username) - 1] = '\0';
    user->admin = true;
    user->last_logon[0] = '\0'; // Empty string for never logged in
}

// Helper to parse user from cJSON object
static bool parse_user_from_json(cJSON *json_user, user_t *user)
{
    cJSON *item;
    
    // Required: USERID
    item = cJSON_GetObjectItem(json_user, "USERID");
    if (!item || !cJSON_IsNumber(item)) {
        ESP_LOGE(TAG, "Missing or invalid USERID");
        return false;
    }
    user->userid = item->valueint;
    
    // Required: PIN
    item = cJSON_GetObjectItem(json_user, "PIN");
    if (!item || !cJSON_IsString(item)) {
        ESP_LOGE(TAG, "Missing or invalid PIN");
        return false;
    }
    strncpy(user->pin, item->valuestring, sizeof(user->pin) - 1);
    user->pin[sizeof(user->pin) - 1] = '\0';
    
    // Optional: FINGERID
    item = cJSON_GetObjectItem(json_user, "FINGERID");
    if (item && cJSON_IsNumber(item)) {
        user->fingerid = item->valueint;
    } else {
        user->fingerid = -1;
    }
    
    // Optional: FACEID
    item = cJSON_GetObjectItem(json_user, "FACEID");
    if (item && cJSON_IsNumber(item)) {
        user->faceid = item->valueint;
    } else {
        user->faceid = -1;
    }
    
    // Optional: USERNAME
    item = cJSON_GetObjectItem(json_user, "USERNAME");
    if (item && cJSON_IsString(item)) {
        strncpy(user->username, item->valuestring, sizeof(user->username) - 1);
        user->username[sizeof(user->username) - 1] = '\0';
    } else {
        snprintf(user->username, sizeof(user->username), "User%d", user->userid);
    }
    
    // Optional: ADMIN
    item = cJSON_GetObjectItem(json_user, "ADMIN");
    if (item && cJSON_IsBool(item)) {
        user->admin = cJSON_IsTrue(item);
    } else {
        user->admin = false;
    }
    
    // Optional: LAST_LOGON
    item = cJSON_GetObjectItem(json_user, "LAST_LOGON");
    if (item && cJSON_IsString(item)) {
        strncpy(user->last_logon, item->valuestring, sizeof(user->last_logon) - 1);
        user->last_logon[sizeof(user->last_logon) - 1] = '\0';
    } else {
        user->last_logon[0] = '\0';
    }
    
    return true;
}

// Helper to create cJSON object from user
static cJSON* create_json_from_user(const user_t *user)
{
    cJSON *json_user = cJSON_CreateObject();
    if (!json_user) {
        return NULL;
    }
    
    cJSON_AddNumberToObject(json_user, "USERID", user->userid);
    
    if (user->fingerid >= 0) {
        cJSON_AddNumberToObject(json_user, "FINGERID", user->fingerid);
    } else {
        cJSON_AddNullToObject(json_user, "FINGERID");
    }
    
    if (user->faceid >= 0) {
        cJSON_AddNumberToObject(json_user, "FACEID", user->faceid);
    } else {
        cJSON_AddNullToObject(json_user, "FACEID");
    }
    
    cJSON_AddStringToObject(json_user, "PIN", user->pin);
    cJSON_AddStringToObject(json_user, "USERNAME", user->username);
    cJSON_AddBoolToObject(json_user, "ADMIN", user->admin);
    
    if (strlen(user->last_logon) > 0) {
        cJSON_AddStringToObject(json_user, "LAST_LOGON", user->last_logon);
    } else {
        cJSON_AddNullToObject(json_user, "LAST_LOGON");
    }
    
    return json_user;
}

// Initialize user store module
esp_err_t user_store_init(const char *path)
{
    if (path != NULL) {
        strncpy(s_file_path, path, sizeof(s_file_path) - 1);
        s_file_path[sizeof(s_file_path) - 1] = '\0';
    }
    
    ESP_LOGI(TAG, "User store initialized with path: %s", s_file_path);
    return ESP_OK;
}

// Load users from JSON file
esp_err_t user_store_load(user_list_t *out_list)
{
    if (!out_list) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Initialize list
    out_list->items = NULL;
    out_list->count = 0;
    out_list->capacity = USER_STORE_MAX_USERS;
    
    // Enable SD card
    ESP_LOGI(TAG, "Enabling SD card for read");
    ch422g_sd_card_enable(I2C_MASTER_NUM, true);
    vTaskDelay(pdMS_TO_TICKS(50)); // Short delay for SD stabilization
    
    // Check if file exists
    struct stat st;
    if (stat(s_file_path, &st) != 0) {
        ESP_LOGW(TAG, "User file not found, creating default");
        
        // Allocate space for one default admin
        out_list->items = (user_t*)malloc(sizeof(user_t) * USER_STORE_MAX_USERS);
        if (!out_list->items) {
            ch422g_sd_card_enable(I2C_MASTER_NUM, false);
            return ESP_ERR_NO_MEM;
        }
        
        create_default_admin(&out_list->items[0]);
        out_list->count = 1;
        
        // Save the default user
        esp_err_t ret = user_store_save(out_list);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ret;
    }
    
    // Read file
    FILE *f = fopen(s_file_path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file: %s", s_file_path);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_FAIL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > 65536) {
        ESP_LOGE(TAG, "Invalid file size: %ld", file_size);
        fclose(f);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_FAIL;
    }
    
    // Read file content
    char *json_str = (char*)malloc(file_size + 1);
    if (!json_str) {
        fclose(f);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_ERR_NO_MEM;
    }
    
    size_t read_size = fread(json_str, 1, file_size, f);
    fclose(f);
    json_str[read_size] = '\0';
    
    ESP_LOGI(TAG, "Read %zu bytes from file", read_size);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    
    if (!root) {
        ESP_LOGE(TAG, "JSON parse error. Creating backup and default file");
        
        // Try to backup corrupted file
        char backup_path[140];
        snprintf(backup_path, sizeof(backup_path), "/sdcard/users_bak.jsn");
        rename(s_file_path, backup_path);
        
        // Create default
        out_list->items = (user_t*)malloc(sizeof(user_t) * USER_STORE_MAX_USERS);
        if (!out_list->items) {
            ch422g_sd_card_enable(I2C_MASTER_NUM, false);
            return ESP_ERR_NO_MEM;
        }
        
        create_default_admin(&out_list->items[0]);
        out_list->count = 1;
        
        esp_err_t ret = user_store_save(out_list);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ret;
    }
    
    // Get users array
    cJSON *users_array = cJSON_GetObjectItem(root, "users");
    if (!users_array || !cJSON_IsArray(users_array)) {
        ESP_LOGE(TAG, "Invalid JSON structure: missing or invalid 'users' array");
        cJSON_Delete(root);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_FAIL;
    }
    
    int array_size = cJSON_GetArraySize(users_array);
    if (array_size > USER_STORE_MAX_USERS) {
        ESP_LOGW(TAG, "User count exceeds maximum, truncating from %d to %d", 
                 array_size, USER_STORE_MAX_USERS);
        array_size = USER_STORE_MAX_USERS;
    }
    
    // Allocate user array
    out_list->items = (user_t*)malloc(sizeof(user_t) * USER_STORE_MAX_USERS);
    if (!out_list->items) {
        cJSON_Delete(root);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_ERR_NO_MEM;
    }
    
    // Parse each user
    int parsed_count = 0;
    for (int i = 0; i < array_size; i++) {
        cJSON *json_user = cJSON_GetArrayItem(users_array, i);
        if (json_user && parse_user_from_json(json_user, &out_list->items[parsed_count])) {
            parsed_count++;
        } else {
            ESP_LOGW(TAG, "Skipping invalid user at index %d", i);
        }
    }
    
    out_list->count = parsed_count;
    cJSON_Delete(root);
    
    ch422g_sd_card_enable(I2C_MASTER_NUM, false);
    
    ESP_LOGI(TAG, "Loaded %zu users from file", out_list->count);
    return ESP_OK;
}

// Save users to JSON file
esp_err_t user_store_save(const user_list_t *list)
{
    if (!list || !list->items) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Saving %zu users to file", list->count);
    
    // Enable SD card
    ch422g_sd_card_enable(I2C_MASTER_NUM, true);
    vTaskDelay(pdMS_TO_TICKS(100)); // Increased delay for filesystem stability
    
    // Create JSON structure
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_ERR_NO_MEM;
    }
    
    cJSON *users_array = cJSON_CreateArray();
    if (!users_array) {
        cJSON_Delete(root);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_ERR_NO_MEM;
    }
    
    // Add users to array
    for (size_t i = 0; i < list->count; i++) {
        cJSON *json_user = create_json_from_user(&list->items[i]);
        if (json_user) {
            cJSON_AddItemToArray(users_array, json_user);
        } else {
            ESP_LOGE(TAG, "Failed to create JSON for user %zu", i);
        }
    }
    
    cJSON_AddItemToObject(root, "users", users_array);
    
    // Serialize to string
    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);
    
    if (!json_str) {
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_ERR_NO_MEM;
    }
    
    // First, try direct write to original file (simpler, no temp file)
    ESP_LOGI(TAG, "Opening file for direct write: %s", s_file_path);
    
    FILE *f = fopen(s_file_path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s (errno: %d)", s_file_path, errno);
        
        // Check mount point
        struct stat st;
        if (stat("/sdcard", &st) != 0) {
            ESP_LOGE(TAG, "/sdcard mount point not accessible");
        } else {
            ESP_LOGI(TAG, "/sdcard is accessible, mode: 0x%lx", (unsigned long)st.st_mode);
        }
        
        // Try to test if we can write any file at all
        ESP_LOGW(TAG, "Testing if SD card allows any write operation...");
        FILE *test = fopen("/sdcard/test_wri.te", "wb");
        if (test) {
            ESP_LOGI(TAG, "Test write file opened successfully!");
            fwrite("test", 1, 4, test);
            fclose(test);
            unlink("/sdcard/test_wri.te");
        } else {
            ESP_LOGE(TAG, "Test write also failed (errno: %d) - SD card may be read-only", errno);
        }
        
        free(json_str);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "File opened successfully, writing %zu bytes", strlen(json_str));
    
    size_t len = strlen(json_str);
    size_t written = fwrite(json_str, 1, len, f);
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    free(json_str);
    
    if (written != len) {
        ESP_LOGE(TAG, "Write error: %zu/%zu bytes", written, len);
        ch422g_sd_card_enable(I2C_MASTER_NUM, false);
        return ESP_FAIL;
    }
    
    ch422g_sd_card_enable(I2C_MASTER_NUM, false);
    
    ESP_LOGI(TAG, "Successfully saved users to file");
    return ESP_OK;
}

// Free user list memory
void user_store_free(user_list_t *list)
{
    if (list && list->items) {
        free(list->items);
        list->items = NULL;
        list->count = 0;
        list->capacity = 0;
    }
}

// Get current file path
const char* user_store_get_path(void)
{
    return s_file_path;
}
