/*
 * User Management UI Module - Implementation
 * LVGL-based user interface for user management
 * 
 * SPDX-FileCopyrightText: 2026 Secure Lock Box System
 * SPDX-License-Identifier: Apache-2.0
 */

#include "user_mgmt_ui.h"
#include "user_store.h"
#include "user_service.h"
#include "lvgl_port.h"
#include "esp_log.h"
#include "ui/ui.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "USER_MGMT_UI";

// UI state and elements
static lv_obj_t *s_screen = NULL;
static lv_obj_t *s_user_dropdown = NULL;
static lv_obj_t *s_userid_label = NULL;
static lv_obj_t *s_username_input = NULL;
static lv_obj_t *s_pin_input = NULL;
static lv_obj_t *s_pin_confirm_input = NULL;
static lv_obj_t *s_admin_switch = NULL;
static lv_obj_t *s_fingerid_label = NULL;
static lv_obj_t *s_faceid_label = NULL;
static lv_obj_t *s_lastlogon_label = NULL;
static lv_obj_t *s_error_label = NULL;

static lv_obj_t *s_btn_add = NULL;
static lv_obj_t *s_btn_save = NULL;
static lv_obj_t *s_btn_delete = NULL;
static lv_obj_t *s_btn_cancel = NULL;
static lv_obj_t *s_btn_close = NULL;
static lv_obj_t *s_keyboard = NULL;
static lv_obj_t *s_current_textarea = NULL;

// Edit Overlay elements
static lv_obj_t *s_edit_overlay = NULL;
static lv_obj_t *s_edit_textarea = NULL;
static lv_obj_t *s_target_textarea = NULL;

// Data
static user_list_t s_user_list = {0};
static int s_current_admin_userid = 1;
static int s_selected_user_index = -1;
static bool s_is_add_mode = false;

// Forward declarations
static void populate_user_dropdown(void);
static void load_user_to_form(int index);
static void clear_form(void);
static void show_error(const char *msg);
static void clear_error(void);
static void textarea_focus_event_cb(lv_event_t *e);
static void show_edit_overlay(lv_obj_t *target_ta, const char *title, bool numeric);
static void show_pin_switch_event_cb(lv_event_t *e);

// Edit overlay event handler
static void edit_overlay_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    bool should_apply = false;
    bool should_close = false;
    
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *label = lv_obj_get_child(obj, 0);
        if (label) {
            const char *btn_text = lv_label_get_text(label);
            if (strcmp(btn_text, "Apply") == 0) {
                should_apply = true;
                should_close = true;
            } else if (strcmp(btn_text, "Cancel") == 0) {
                should_close = true;
            }
        }
    } else if (code == LV_EVENT_READY) {
        should_apply = true;
        should_close = true;
    } else if (code == LV_EVENT_CANCEL) {
        should_close = true;
    }
    
    if (should_apply) {
        // Copy text back to target
        if (s_target_textarea && s_edit_textarea) {
            lv_textarea_set_text(s_target_textarea, lv_textarea_get_text(s_edit_textarea));
        }
    }
    
    if (should_close && s_edit_overlay) {
        lv_obj_del(s_edit_overlay);
        s_edit_overlay = NULL;
        s_edit_textarea = NULL;
        s_target_textarea = NULL;
    }
}

static void show_pin_switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool show = lv_obj_has_state(sw, LV_STATE_CHECKED);
    
    if (s_pin_input) lv_textarea_set_password_mode(s_pin_input, !show);
    if (s_pin_confirm_input) lv_textarea_set_password_mode(s_pin_confirm_input, !show);
    
    ESP_LOGI(TAG, "PIN visibility toggled: %s", show ? "Visible" : "Hidden");
}

static void show_edit_overlay(lv_obj_t *target_ta, const char *title, bool numeric)
{
    s_target_textarea = target_ta;
    
    // Create overlay container
    s_edit_overlay = lv_obj_create(lv_screen_active());
    lv_obj_set_size(s_edit_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_edit_overlay, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(s_edit_overlay, 0, 0);
    lv_obj_set_style_radius(s_edit_overlay, 0, 0);
    lv_obj_clear_flag(s_edit_overlay, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    lv_obj_t *lbl_title = lv_label_create(s_edit_overlay);
    lv_label_set_text(lbl_title, title);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_26, 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Edit textarea
    s_edit_textarea = lv_textarea_create(s_edit_overlay);
    lv_obj_set_size(s_edit_textarea, LV_PCT(80), 60);
    lv_obj_align(s_edit_textarea, LV_ALIGN_TOP_MID, 0, 70);
    lv_textarea_set_one_line(s_edit_textarea, true);
    lv_textarea_set_text(s_edit_textarea, lv_textarea_get_text(target_ta));
    lv_textarea_set_password_mode(s_edit_textarea, lv_textarea_get_password_mode(target_ta));
    lv_textarea_set_max_length(s_edit_textarea, lv_textarea_get_max_length(target_ta));
    lv_obj_set_style_text_font(s_edit_textarea, &lv_font_montserrat_26, 0);
    
    if (numeric) {
        lv_textarea_set_accepted_chars(s_edit_textarea, "0123456789");
    }
    
    // Keyboard
    lv_obj_t *kb = lv_keyboard_create(s_edit_overlay);
    lv_obj_set_size(kb, LV_PCT(100), LV_PCT(55));
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(kb, s_edit_textarea);
    lv_keyboard_set_mode(kb, numeric ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
    
    // Increase button text size
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_26, LV_PART_ITEMS);
    
    // Add event callback to textarea for keyboard events
    lv_obj_add_event_cb(s_edit_textarea, edit_overlay_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(s_edit_textarea, edit_overlay_event_cb, LV_EVENT_CANCEL, NULL);
    
    // Buttons
    lv_obj_t *btn_cont = lv_obj_create(s_edit_overlay);
    lv_obj_set_size(btn_cont, LV_PCT(100), 70);
    lv_obj_align_to(btn_cont, s_edit_textarea, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(btn_cont, 0, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    
    lv_obj_t *btn_apply = lv_button_create(btn_cont);
    lv_obj_set_size(btn_apply, 120, 45);
    lv_obj_set_style_bg_color(btn_apply, lv_color_hex(0x2196F3), 0);
    lv_obj_t *apply_label = lv_label_create(btn_apply);
    lv_label_set_text(apply_label, "Apply");
    lv_obj_set_style_text_font(apply_label, &lv_font_montserrat_26, 0);
    lv_obj_center(apply_label);
    lv_obj_add_event_cb(btn_apply, edit_overlay_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_cancel_edit = lv_button_create(btn_cont);
    lv_obj_set_size(btn_cancel_edit, 120, 45);
    lv_obj_set_style_bg_color(btn_cancel_edit, lv_color_hex(0x757575), 0);
    lv_obj_set_style_margin_left(btn_cancel_edit, 20, 0);
    lv_obj_t *cancel_edit_label = lv_label_create(btn_cancel_edit);
    lv_label_set_text(cancel_edit_label, "Cancel");
    lv_obj_set_style_text_font(cancel_edit_label, &lv_font_montserrat_26, 0);
    lv_obj_center(cancel_edit_label);
    lv_obj_add_event_cb(btn_cancel_edit, edit_overlay_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_group_focus_obj(s_edit_textarea);
}

// Keyboard event handler
static void textarea_focus_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    
    if (code == LV_EVENT_CLICKED) {
        const char *title = "Edit Field";
        bool numeric = false;
        
        if (ta == s_username_input) title = "Edit Username";
        else if (ta == s_pin_input) {
            title = "Edit PIN";
            numeric = true;
        }
        else if (ta == s_pin_confirm_input) {
            title = "Confirm PIN";
            numeric = true;
        }
        
        show_edit_overlay(ta, title, numeric);
        ESP_LOGI(TAG, "Edit overlay shown for: %s", title);
    }
}

// Event handler for delete confirmation
static void delete_confirm_event_cb(lv_event_t *ev)
{
    if (lv_event_get_code(ev) != LV_EVENT_CLICKED) {
        return;
    }
    
    esp_err_t ret = user_service_delete(&s_user_list, s_selected_user_index, s_current_admin_userid);
    
    if (ret == ESP_OK) {
        ret = user_store_save(&s_user_list);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "User deleted successfully");
            populate_user_dropdown();
            s_selected_user_index = -1;
            clear_form();
            show_error("User deleted successfully");
            lv_obj_add_state(s_btn_delete, LV_STATE_DISABLED);
        } else {
            show_error("Failed to save after delete");
        }
    } else {
        show_error("Cannot delete this user");
    }
    
    lv_msgbox_close(lv_obj_get_parent(lv_event_get_target(ev)));
}

// Event handlers
static void dropdown_changed_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        clear_error();
        s_is_add_mode = false;
        
        uint16_t selected = lv_dropdown_get_selected(s_user_dropdown);
        if (selected < s_user_list.count) {
            s_selected_user_index = selected;
            load_user_to_form(selected);
            
            // Enable/disable buttons
            lv_obj_clear_state(s_btn_save, LV_STATE_DISABLED);
            lv_obj_clear_state(s_btn_delete, LV_STATE_DISABLED);
        }
    }
}

static void add_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        clear_error();
        s_is_add_mode = true;
        s_selected_user_index = -1;
        clear_form();
        
        // Enable save, disable delete
        lv_obj_clear_state(s_btn_save, LV_STATE_DISABLED);
        lv_obj_add_state(s_btn_delete, LV_STATE_DISABLED);
        
        ESP_LOGI(TAG, "Add new user mode activated");
    }
}

static void save_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }
    
    clear_error();
    
    // Get form values
    const char *username = lv_textarea_get_text(s_username_input);
    const char *pin = lv_textarea_get_text(s_pin_input);
    const char *pin_confirm = lv_textarea_get_text(s_pin_confirm_input);
    bool is_admin = lv_obj_has_state(s_admin_switch, LV_STATE_CHECKED);
    
    // Validate inputs
    if (!username || strlen(username) == 0) {
        show_error("Username cannot be empty");
        return;
    }
    
    if (!user_service_validate_username(username)) {
        show_error("Invalid username format");
        return;
    }
    
    if (!pin || strlen(pin) == 0) {
        show_error("PIN cannot be empty");
        return;
    }
    
    if (!user_service_validate_pin(pin)) {
        show_error("PIN must be 4-8 digits");
        return;
    }
    
    if (strcmp(pin, pin_confirm) != 0) {
        show_error("PINs do not match");
        return;
    }
    
    user_t user;
    memset(&user, 0, sizeof(user));
    strncpy(user.username, username, sizeof(user.username) - 1);
    strncpy(user.pin, pin, sizeof(user.pin) - 1);
    user.admin = is_admin;
    
    esp_err_t ret;
    
    if (s_is_add_mode) {
        // Add new user
        int new_index;
        ret = user_service_add(&s_user_list, &user, &new_index);
        
        if (ret == ESP_OK) {
            // Save to file
            ret = user_store_save(&s_user_list);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "User added successfully");
                populate_user_dropdown();
                lv_dropdown_set_selected(s_user_dropdown, new_index);
                s_selected_user_index = new_index;
                s_is_add_mode = false;
                load_user_to_form(new_index);
                show_error("User added successfully");
            } else {
                show_error("Failed to save to file");
            }
        } else if (ret == ESP_ERR_NO_MEM) {
            show_error("User list is full");
        } else {
            show_error("Failed to add user");
        }
    } else {
        // Update existing user
        if (s_selected_user_index < 0 || s_selected_user_index >= (int)s_user_list.count) {
            show_error("No user selected");
            return;
        }
        
        ret = user_service_update(&s_user_list, s_selected_user_index, &user, s_current_admin_userid);
        
        if (ret == ESP_OK) {
            // Save to file
            ret = user_store_save(&s_user_list);
            if (ret == ESP_OK) {
                ESP_LOGI(TAG, "User updated successfully");
                populate_user_dropdown();
                lv_dropdown_set_selected(s_user_dropdown, s_selected_user_index);
                load_user_to_form(s_selected_user_index);
                show_error("User updated successfully");
            } else {
                show_error("Failed to save to file");
            }
        } else {
            show_error("Failed to update user");
        }
    }
}

static void delete_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) {
        return;
    }
    
    clear_error();
    
    if (s_selected_user_index < 0 || s_selected_user_index >= (int)s_user_list.count) {
        show_error("No user selected");
        return;
    }
    
    // Create confirmation dialog
    lv_obj_t *mbox = lv_msgbox_create(s_screen);
    lv_msgbox_add_title(mbox, "Confirm Delete");
    
    char msg[128];
    user_t *user = &s_user_list.items[s_selected_user_index];
    snprintf(msg, sizeof(msg), "Delete user '%s' (ID %d)?\nThis cannot be undone.", 
             user->username, user->userid);
    lv_msgbox_add_text(mbox, msg);
    
    lv_msgbox_add_close_button(mbox);
    lv_obj_t *btn = lv_msgbox_add_footer_button(mbox, "Delete");
    lv_obj_add_event_cb(btn, delete_confirm_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_center(mbox);
}

static void cancel_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        clear_error();
        s_is_add_mode = false;
        
        if (s_selected_user_index >= 0 && s_selected_user_index < (int)s_user_list.count) {
            load_user_to_form(s_selected_user_index);
        } else {
            clear_form();
        }
        
        ESP_LOGI(TAG, "Changes cancelled");
    }
}

static void close_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        user_mgmt_ui_close();
    }
}

// Helper functions
static void populate_user_dropdown(void)
{
    if (!s_user_dropdown) return;
    
    lv_dropdown_clear_options(s_user_dropdown);
    
    for (size_t i = 0; i < s_user_list.count; i++) {
        lv_dropdown_add_option(s_user_dropdown, s_user_list.items[i].username, i);
    }
    
    ESP_LOGI(TAG, "Populated dropdown with %zu users", s_user_list.count);
}

static void load_user_to_form(int index)
{
    if (index < 0 || index >= (int)s_user_list.count) {
        return;
    }
    
    user_t *user = &s_user_list.items[index];
    
    // Set field values
    char buf[64];
    snprintf(buf, sizeof(buf), "ID: %d", user->userid);
    lv_label_set_text(s_userid_label, buf);
    
    lv_textarea_set_text(s_username_input, user->username);
    lv_textarea_set_text(s_pin_input, user->pin);
    lv_textarea_set_text(s_pin_confirm_input, user->pin);
    
    if (user->admin) {
        lv_obj_add_state(s_admin_switch, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_admin_switch, LV_STATE_CHECKED);
    }
    
    snprintf(buf, sizeof(buf), "Finger: %s", 
             user->fingerid >= 0 ? "Enrolled" : "Not enrolled");
    lv_label_set_text(s_fingerid_label, buf);
    
    snprintf(buf, sizeof(buf), "Face: %s", 
             user->faceid >= 0 ? "Enrolled" : "Not enrolled");
    lv_label_set_text(s_faceid_label, buf);
    
    const char *logon_text = (strlen(user->last_logon) > 0) ? user->last_logon : "Never";
    snprintf(buf, sizeof(buf), "Last Logon: %s", logon_text);
    lv_label_set_text(s_lastlogon_label, buf);
    
    ESP_LOGI(TAG, "Loaded user '%s' to form", user->username);
}

static void clear_form(void)
{
    lv_label_set_text(s_userid_label, "ID: (new)");
    lv_textarea_set_text(s_username_input, "");
    lv_textarea_set_text(s_pin_input, "");
    lv_textarea_set_text(s_pin_confirm_input, "");
    lv_obj_clear_state(s_admin_switch, LV_STATE_CHECKED);
    lv_label_set_text(s_fingerid_label, "Finger: Not enrolled");
    lv_label_set_text(s_faceid_label, "Face: Not enrolled");
    lv_label_set_text(s_lastlogon_label, "Last Logon: Never");
}

static void show_error(const char *msg)
{
    if (s_error_label && msg) {
        lv_label_set_text(s_error_label, msg);
        lv_obj_clear_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
    }
}

static void clear_error(void)
{
    if (s_error_label) {
        lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
    }
}

// Public functions
lv_obj_t* user_mgmt_ui_create(int current_admin_userid)
{
    s_current_admin_userid = current_admin_userid;
    
    // Create screen
    s_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_screen, lv_color_hex(0x1a1a1a), 0);
    lv_obj_clear_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(s_screen, LV_SCROLLBAR_MODE_OFF);
    
    // Header
    lv_obj_t *header = lv_obj_create(s_screen);
    lv_obj_set_size(header, LV_PCT(100), 60);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "User Management");
    //lv_obj_set_style_text_color(title, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x8719e0), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_26, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 20, 0);
    
    s_btn_close = lv_button_create(header);
    lv_obj_set_size(s_btn_close, 100, 40);
    lv_obj_align(s_btn_close, LV_ALIGN_RIGHT_MID, -20, 0);
    lv_obj_t *close_label = lv_label_create(s_btn_close);
    lv_label_set_text(close_label, "Back");
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_26, 0);
    lv_obj_center(close_label);
    lv_obj_add_event_cb(s_btn_close, close_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Main container
    lv_obj_t *main_cont = lv_obj_create(s_screen);
    lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100) - 130);
    lv_obj_align(main_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    lv_obj_set_style_bg_opa(main_cont, 0, 0);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);
    
    // Left pane - user list
    lv_obj_t *left_pane = lv_obj_create(main_cont);
    lv_obj_set_size(left_pane, 280, LV_PCT(100));
    lv_obj_set_style_bg_color(left_pane, lv_color_hex(0x252525), 0);
    lv_obj_set_style_border_width(left_pane, 0, 0);
    lv_obj_set_style_radius(left_pane, 0, 0);
    
    lv_obj_t *list_label = lv_label_create(left_pane);
    lv_label_set_text(list_label, "Registered Users:");
    lv_obj_set_style_text_font(list_label, &lv_font_montserrat_26, 0);
    lv_obj_align(list_label, LV_ALIGN_TOP_LEFT, 20, 20);
    
    s_user_dropdown = lv_dropdown_create(left_pane);
    lv_obj_set_width(s_user_dropdown, 240);
    lv_obj_set_style_text_font(s_user_dropdown, &lv_font_montserrat_26, 0);
    lv_obj_t * list = lv_dropdown_get_list(s_user_dropdown);
    lv_obj_set_style_text_font(list, &lv_font_montserrat_24, 0);
    lv_obj_align(s_user_dropdown, LV_ALIGN_TOP_LEFT, 20, 50);
    lv_obj_add_event_cb(s_user_dropdown, dropdown_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Biometric Info Section (in left pane)
    lv_obj_t *bio_cont = lv_obj_create(left_pane);
    lv_obj_set_size(bio_cont, 240, 200);
    lv_obj_align(bio_cont, LV_ALIGN_TOP_LEFT, 20, 110);
    lv_obj_set_flex_flow(bio_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(bio_cont, 10, 0);
    lv_obj_set_style_bg_opa(bio_cont, 0, 0);
    lv_obj_set_style_border_width(bio_cont, 0, 0);
    lv_obj_set_style_pad_all(bio_cont, 0, 0);
    
    s_userid_label = lv_label_create(bio_cont);
    lv_label_set_text(s_userid_label, "ID: (new)");
    lv_obj_set_style_text_color(s_userid_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(s_userid_label, &lv_font_montserrat_24, 0);
    
    s_fingerid_label = lv_label_create(bio_cont);
    lv_label_set_text(s_fingerid_label, "Finger: Not enrolled");
    lv_obj_set_style_text_color(s_fingerid_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(s_fingerid_label, &lv_font_montserrat_24, 0);
    
    s_faceid_label = lv_label_create(bio_cont);
    lv_label_set_text(s_faceid_label, "Face: Not enrolled");
    lv_obj_set_style_text_color(s_faceid_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(s_faceid_label, &lv_font_montserrat_24, 0);
    
    s_lastlogon_label = lv_label_create(bio_cont);
    lv_label_set_text(s_lastlogon_label, "Last Logon: Never");
    lv_obj_set_style_text_color(s_lastlogon_label, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_style_text_font(s_lastlogon_label, &lv_font_montserrat_24, 0);
    
    // Right pane - form
    lv_obj_t *right_pane = lv_obj_create(main_cont);
    lv_obj_set_flex_grow(right_pane, 1);
    lv_obj_set_height(right_pane, LV_PCT(100));
    lv_obj_set_style_bg_color(right_pane, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(right_pane, 0, 0);
    lv_obj_set_flex_flow(right_pane, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(right_pane, 10, 0);
    lv_obj_set_style_pad_row(right_pane, 10, 0);
    lv_obj_set_scrollbar_mode(right_pane, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(right_pane, LV_OBJ_FLAG_SCROLLABLE);
    
    // Form Helper Macro-like logic for rows
    #define CREATE_FORM_ROW(parent, label_text) \
        lv_obj_t *row = lv_obj_create(parent); \
        lv_obj_set_size(row, LV_PCT(100), 90); \
        lv_obj_set_style_bg_opa(row, 0, 0); \
        lv_obj_set_style_border_width(row, 0, 0); \
        lv_obj_set_style_pad_all(row, 0, 0); \
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN); \
        lv_obj_set_style_pad_row(row, 5, 0); \
        lv_obj_t *lbl = lv_label_create(row); \
        lv_label_set_text(lbl, label_text); \
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xaaaaaa), 0); \
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
        
    // Username Row
    {
        CREATE_FORM_ROW(right_pane, "Username");
        s_username_input = lv_textarea_create(row);
        lv_obj_set_size(s_username_input, LV_PCT(100), 50);
        lv_obj_set_style_text_font(s_username_input, &lv_font_montserrat_26, 0);
        lv_textarea_set_one_line(s_username_input, true);
        lv_textarea_set_placeholder_text(s_username_input, "Enter username");
        lv_textarea_set_max_length(s_username_input, USER_STORE_MAX_USERNAME_LEN);
        lv_obj_add_event_cb(s_username_input, textarea_focus_event_cb, LV_EVENT_CLICKED, NULL);
    }
    
    // PIN Row (Two columns for PIN and Confirm)
    lv_obj_t *pin_row_cont = lv_obj_create(right_pane);
    lv_obj_set_size(pin_row_cont, LV_PCT(100), 90);
    lv_obj_set_style_bg_opa(pin_row_cont, 0, 0);
    lv_obj_set_style_border_width(pin_row_cont, 0, 0);
    lv_obj_set_style_pad_all(pin_row_cont, 0, 0);
    lv_obj_set_flex_flow(pin_row_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pin_row_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(pin_row_cont, 20, 0);
    
    {
        lv_obj_t *pin_col = lv_obj_create(pin_row_cont);
        lv_obj_set_size(pin_col, 200, 90);
        lv_obj_set_style_bg_opa(pin_col, 0, 0);
        lv_obj_set_style_border_width(pin_col, 0, 0);
        lv_obj_set_style_pad_all(pin_col, 0, 0);
        lv_obj_set_flex_flow(pin_col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(pin_col, 5, 0);
        
        lv_obj_t *lbl = lv_label_create(pin_col);
        lv_label_set_text(lbl, "PIN");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xaaaaaa), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
        
        s_pin_input = lv_textarea_create(pin_col);
        lv_obj_set_size(s_pin_input, 200, 50);
        lv_obj_set_style_text_font(s_pin_input, &lv_font_montserrat_26, 0);
        lv_textarea_set_one_line(s_pin_input, true);
        lv_textarea_set_password_mode(s_pin_input, true);
        lv_textarea_set_max_length(s_pin_input, USER_STORE_MAX_PIN_LEN);
        lv_obj_add_event_cb(s_pin_input, textarea_focus_event_cb, LV_EVENT_CLICKED, NULL);
    }
    
    {
        lv_obj_t *confirm_col = lv_obj_create(pin_row_cont);
        lv_obj_set_size(confirm_col, 200, 90);
        lv_obj_set_style_bg_opa(confirm_col, 0, 0);
        lv_obj_set_style_border_width(confirm_col, 0, 0);
        lv_obj_set_style_pad_all(confirm_col, 0, 0);
        lv_obj_set_flex_flow(confirm_col, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_row(confirm_col, 5, 0);
        
        lv_obj_t *lbl = lv_label_create(confirm_col);
        lv_label_set_text(lbl, "Confirm PIN");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xaaaaaa), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
        
        s_pin_confirm_input = lv_textarea_create(confirm_col);
        lv_obj_set_size(s_pin_confirm_input, 200, 50);
        lv_obj_set_style_text_font(s_pin_confirm_input, &lv_font_montserrat_26, 0);
        lv_textarea_set_one_line(s_pin_confirm_input, true);
        lv_textarea_set_password_mode(s_pin_confirm_input, true);
        lv_textarea_set_max_length(s_pin_confirm_input, USER_STORE_MAX_PIN_LEN);
        lv_obj_add_event_cb(s_pin_confirm_input, textarea_focus_event_cb, LV_EVENT_CLICKED, NULL);
    }
    
    // Switch controls row
    lv_obj_t *switch_row_cont = lv_obj_create(right_pane);
    lv_obj_set_size(switch_row_cont, LV_PCT(100), 50); // Adjust height as needed
    lv_obj_set_style_bg_opa(switch_row_cont, 0, 0);
    lv_obj_set_style_border_width(switch_row_cont, 0, 0);
    lv_obj_set_style_pad_all(switch_row_cont, 0, 0);
    lv_obj_set_flex_flow(switch_row_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(switch_row_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Show PIN Switch
    {
        lv_obj_t *pin_switch_wrap = lv_obj_create(switch_row_cont);
        lv_obj_set_size(pin_switch_wrap, LV_SIZE_CONTENT, LV_PCT(100));
        lv_obj_set_style_bg_opa(pin_switch_wrap, 0, 0);
        lv_obj_set_style_border_width(pin_switch_wrap, 0, 0);
        lv_obj_set_style_pad_all(pin_switch_wrap, 0, 0);
        lv_obj_set_flex_flow(pin_switch_wrap, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(pin_switch_wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(pin_switch_wrap, 10, 0);

        lv_obj_t *lbl = lv_label_create(pin_switch_wrap);
        lv_label_set_text(lbl, "Show PIN");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xaaaaaa), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);

        lv_obj_t *pin_sw = lv_switch_create(pin_switch_wrap);
        lv_obj_add_event_cb(pin_sw, show_pin_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // Administrator Switch
    {
        lv_obj_t *admin_switch_wrap = lv_obj_create(switch_row_cont);
        lv_obj_set_size(admin_switch_wrap, LV_SIZE_CONTENT, LV_PCT(100));
        lv_obj_set_style_bg_opa(admin_switch_wrap, 0, 0);
        lv_obj_set_style_border_width(admin_switch_wrap, 0, 0);
        lv_obj_set_style_pad_all(admin_switch_wrap, 0, 0);
        lv_obj_set_flex_flow(admin_switch_wrap, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(admin_switch_wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(admin_switch_wrap, 10, 0);

        lv_obj_t *lbl = lv_label_create(admin_switch_wrap);
        lv_label_set_text(lbl, "Admin");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xaaaaaa), 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
        
        s_admin_switch = lv_switch_create(admin_switch_wrap);
    }
    
    // Error label
    s_error_label = lv_label_create(right_pane);
    lv_label_set_text(s_error_label, "");
    lv_obj_set_style_text_color(s_error_label, lv_color_hex(0xff4444), 0);
    lv_obj_set_style_text_font(s_error_label, &lv_font_montserrat_26, 0);
    lv_obj_add_flag(s_error_label, LV_OBJ_FLAG_HIDDEN);
    
    // Bottom button row
    lv_obj_t *button_row = lv_obj_create(s_screen);
    lv_obj_set_size(button_row, LV_PCT(100), 70);
    lv_obj_align(button_row, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_flex_flow(button_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(button_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(button_row, 20, 0);
    lv_obj_set_style_bg_color(button_row, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_border_width(button_row, 0, 0);
    lv_obj_set_style_radius(button_row, 0, 0);
    
    // Add button
    s_btn_add = lv_button_create(button_row);
    lv_obj_set_size(s_btn_add, 160, 50);
    lv_obj_t *add_label = lv_label_create(s_btn_add);
    lv_label_set_text(add_label, "Add New");
    lv_obj_set_style_text_font(add_label, &lv_font_montserrat_26, 0);
    lv_obj_center(add_label);
    lv_obj_add_event_cb(s_btn_add, add_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Save button
    s_btn_save = lv_button_create(button_row);
    lv_obj_set_size(s_btn_save, 160, 50);
    lv_obj_set_style_bg_color(s_btn_save, lv_color_hex(0x4CAF50), 0);
    lv_obj_t *save_label = lv_label_create(s_btn_save);
    lv_label_set_text(save_label, "Save");
    lv_obj_set_style_text_font(save_label, &lv_font_montserrat_26, 0);
    lv_obj_center(save_label);
    lv_obj_add_event_cb(s_btn_save, save_button_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(s_btn_save, LV_STATE_DISABLED);
    
    // Delete button
    s_btn_delete = lv_button_create(button_row);
    lv_obj_set_size(s_btn_delete, 160, 50);
    lv_obj_set_style_bg_color(s_btn_delete, lv_color_hex(0xF44336), 0);
    lv_obj_t *delete_label = lv_label_create(s_btn_delete);
    lv_label_set_text(delete_label, "Delete");
    lv_obj_set_style_text_font(delete_label, &lv_font_montserrat_26, 0);
    lv_obj_center(delete_label);
    lv_obj_add_event_cb(s_btn_delete, delete_button_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_state(s_btn_delete, LV_STATE_DISABLED);
    
    // Cancel button
    s_btn_cancel = lv_button_create(button_row);
    lv_obj_set_size(s_btn_cancel, 160, 50);
    lv_obj_set_style_bg_color(s_btn_cancel, lv_color_hex(0x757575), 0);
    lv_obj_t *cancel_label = lv_label_create(s_btn_cancel);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_set_style_text_font(cancel_label, &lv_font_montserrat_26, 0);
    lv_obj_center(cancel_label);
    lv_obj_add_event_cb(s_btn_cancel, cancel_button_event_cb, LV_EVENT_CLICKED, NULL);
    
    ESP_LOGI(TAG, "User management UI created with improved layout");
    
    return s_screen;
}

void user_mgmt_ui_show(void)
{
    if (!s_screen) {
        ESP_LOGE(TAG, "Screen not created");
        return;
    }
    
    // Load user data
    esp_err_t ret = user_store_load(&s_user_list);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load user data");
        return;
    }
    
    // Populate dropdown
    populate_user_dropdown();
    
    // Clear form
    clear_form();
    s_selected_user_index = -1;
    s_is_add_mode = false;
    
    // Load screen
    lv_screen_load(s_screen);
    
    ESP_LOGI(TAG, "User management UI shown with %zu users", s_user_list.count);
}

void user_mgmt_ui_close(void)
{
    // Free user list
    user_store_free(&s_user_list);
    
    ESP_LOGI(TAG, "User management UI closed");
    
    // Return to home screen
    if (ui_screen_home) {
        lv_screen_load(ui_screen_home);
    }
}

lv_obj_t* user_mgmt_ui_get_screen(void)
{
    return s_screen;
}
