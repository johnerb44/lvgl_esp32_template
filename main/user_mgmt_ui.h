/*
 * User Management UI Module - Header
 * LVGL-based user interface for user management
 * 
 * SPDX-FileCopyrightText: 2026 Secure Lock Box System
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef USER_MGMT_UI_H
#define USER_MGMT_UI_H

#include "lvgl.h"
#include "user_store.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create and initialize user management screen
 * 
 * @param parent Parent screen object (typically the main screen)
 * @param current_admin_userid User ID of currently logged in admin
 * @return lv_obj_t* Pointer to created screen object
 */
lv_obj_t* user_mgmt_ui_create(int current_admin_userid);

/**
 * @brief Show the user management screen
 * 
 * Loads user data and populates the UI
 */
void user_mgmt_ui_show(void);

/**
 * @brief Close the user management screen
 * 
 * Saves data if needed and returns to previous screen
 */
void user_mgmt_ui_close(void);

/**
 * @brief Get the user management screen object
 * 
 * @return lv_obj_t* Pointer to screen object or NULL if not created
 */
lv_obj_t* user_mgmt_ui_get_screen(void);

#ifdef __cplusplus
}
#endif

#endif // USER_MGMT_UI_H
