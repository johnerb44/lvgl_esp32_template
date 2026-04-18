# User Management Integration Guide

This document provides guidance on integrating the user management functionality into your existing LVGL screens.

## Overview

The user management system consists of three layers:
1. **user_store** - JSON-based data persistence using cJSON
2. **user_service** - Business logic and validation
3. **user_mgmt_ui** - LVGL-based user interface

## How to Navigate to User Management Screen

### From Any LVGL Screen

Add a button or menu item to navigate to the user management screen:

```c
// Example: Add to your screen creation function
static void open_user_mgmt_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        // Lock LVGL mutex
        if (lvgl_port_lock(-1)) {
            // Show user management screen
            user_mgmt_ui_show();  // This loads data and switches screen
            lvgl_port_unlock();
        }
    }
}

// In your screen creation function:
lv_obj_t *btn_user_mgmt = lv_button_create(parent);
lv_obj_set_size(btn_user_mgmt, 200, 50);
lv_obj_t *label = lv_label_create(btn_user_mgmt);
lv_label_set_text(label, "User Management");
lv_obj_center(label);
lv_obj_add_event_cb(btn_user_mgmt, open_user_mgmt_event_cb, LV_EVENT_CLICKED, NULL);
```

### Return from User Management Screen

To implement proper navigation back from the user management screen, modify `user_mgmt_ui_close()` in `user_mgmt_ui.c`:

```c
void user_mgmt_ui_close(void)
{
    // Free user list
    user_store_free(&s_user_list);
    
    ESP_LOGI(TAG, "User management UI closed");
    
    // Load the screen you want to return to
    // For example, return to main menu:
    if (lvgl_port_lock(-1)) {
        lv_scr_load_anim(ui_screen_main, LV_SCR_LOAD_ANIM_FADE_OUT, 300, 0, false);
        lvgl_port_unlock();
    }
}
```

## Example Integration in ui_screen_main.c

Here's a complete example of adding a "User Management" button to the main screen:

```c
// Add to ui_screen_main.c

#include "user_mgmt_ui.h"
#include "lvgl_port.h"

// In ui_screen_main_create() function, add:

// Create User Management button
lv_obj_t *btn_user_mgmt = lv_button_create(ui_screen_main);
lv_obj_set_size(btn_user_mgmt, 240, 60);
lv_obj_align(btn_user_mgmt, LV_ALIGN_CENTER, 0, 100);  // Adjust position as needed
lv_obj_set_style_bg_color(btn_user_mgmt, lv_color_hex(0x0066cc), 0);

lv_obj_t *label = lv_label_create(btn_user_mgmt);
lv_label_set_text(label, LV_SYMBOL_SETTINGS " User Management");
lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
lv_obj_center(label);

// Event handler function (must be defined outside, not as lambda)
static void user_mgmt_btn_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (lvgl_port_lock(-1)) {
            user_mgmt_ui_show();
            lvgl_port_unlock();
        }
    }
}

// Add event handler
lv_obj_add_event_cb(btn_user_mgmt, user_mgmt_btn_cb, LV_EVENT_CLICKED, NULL);
```

## Direct Testing

To test the user management screen directly from `main.c`:

```c
// In app_main(), after ui_init():
if (lvgl_port_lock(-1)) {
    ui_init();
    
    // Load user management screen directly
    lv_scr_load(ui_screen_user_mgmt);
    user_mgmt_ui_show();  // Load user data
    
    lvgl_port_unlock();
}
```

## User Data File Location

- Default path: `/sdcard/users.jsn`
- The file is created automatically with a default admin user (username: "Admin", PIN: "1234")
- File format follows the JSON schema defined in `json_user_schema.md`
- Backup file: `/sdcard/users_bak.jsn` (created if original file is corrupted)
- Note: FAT32 8.3 filename format required for SD card compatibility

## Text Input and On-Screen Keyboard

The user management UI includes an integrated LVGL keyboard that automatically appears when text fields are clicked:

### Keyboard Behavior
- **Username field**: Displays full text keyboard (lowercase mode)
- **PIN fields**: Displays number-only keyboard
- **Auto-hide**: Keyboard disappears when "OK" or "Close" button on keyboard is pressed
- **Screen position**: Keyboard occupies bottom 40% of screen when visible

### Programmatic Keyboard Control

If you need to customize keyboard behavior, the keyboard is created in `user_mgmt_ui_create()`:

```c
// The keyboard is created as:
s_keyboard = lv_keyboard_create(s_screen);
lv_obj_set_size(s_keyboard, LV_PCT(100), LV_PCT(40));
lv_obj_align(s_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
lv_obj_add_flag(s_keyboard, LV_OBJ_FLAG_HIDDEN);  // Initially hidden

// Textareas trigger keyboard on click:
lv_obj_add_event_cb(s_username_input, textarea_focus_event_cb, LV_EVENT_CLICKED, NULL);
```

### Keyboard Customization

To modify keyboard appearance or behavior, edit `user_mgmt_ui.c`:

```c
// In textarea_focus_event_cb(), you can customize:
if (code == LV_EVENT_CLICKED) {
    // Set keyboard mode
    if (ta == s_pin_input || ta == s_pin_confirm_input) {
        lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_NUMBER);
    } else {
        lv_keyboard_set_mode(s_keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
        // Or use: LV_KEYBOARD_MODE_TEXT_UPPER, LV_KEYBOARD_MODE_SPECIAL
    }
}
```

## Important Notes

1. **Admin Access**: Only users with admin privileges should be able to access user management
2. **Thread Safety**: Always wrap LVGL API calls with `lvgl_port_lock()` / `lvgl_port_unlock()`
3. **SD Card**: The SD card must be initialized before using user management functions
4. **Current Admin ID**: Pass the logged-in admin's user ID to `user_mgmt_ui_create()` to prevent self-deletion

## Example: Full Navigation Flow

```c
// 1. From login screen -> main menu (after successful admin login)
int current_user_id = authenticated_user_id;

// 2. Main menu -> user management
if (user_is_admin) {
    lv_obj_add_flag(btn_user_mgmt, LV_OBJ_FLAG_CLICKABLE);  // Enable button
} else {
    lv_obj_add_flag(btn_user_mgmt, LV_OBJ_FLAG_HIDDEN);  // Hide from non-admins
}

// 3. User management button click
static void user_mgmt_btn_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (lvgl_port_lock(-1)) {
            user_mgmt_ui_show();
            lvgl_port_unlock();
        }
    }
}
```

## API Reference

### user_store.h
- `user_store_init(path)` - Initialize with custom or default path
- `user_store_load(list)` - Load users from SD card
- `user_store_save(list)` - Save users to SD card
- `user_store_free(list)` - Free memory

### user_service.h
- `user_service_add(list, user, out_index)` - Add new user
- `user_service_update(list, index, user, admin_id)` - Update existing user
- `user_service_delete(list, index, admin_id)` - Delete user
- `user_service_find_by_username(list, username)` - Search by username
- `user_service_find_by_userid(list, userid)` - Search by ID

### user_mgmt_ui.h
- `user_mgmt_ui_create(admin_userid)` - Create screen (called by ui_init)
- `user_mgmt_ui_show()` - Load data and show screen
- `user_mgmt_ui_close()` - Save and return to previous screen
- `user_mgmt_ui_get_screen()` - Get screen object

## Troubleshooting

### Screen doesn't load
- Ensure `ui_init()` has been called
- Check that `lvgl_port_lock()` is held before screen operations

### File not found errors
- Verify SD card is initialized and mounted at `/sdcard`
- Check SD card connections and CH422G configuration
- Default file `/sdcard/users.jsn` is created automatically on first run

### Users not saving
- **Known Issue**: FAT filesystem on ESP32 has limitations with new file creation
- Current implementation uses direct file overwrite (not atomic writes)
- Check SD card write permissions
- Verify sufficient space on SD card (needs ~5KB for 64 users)
- Check ESP logs for detailed error messages
- Ensure 100ms delay after SD card enable is sufficient (adjust in `user_store.c` if needed)

### UI appears blank
- Ensure `user_mgmt_ui_show()` is called after loading the screen
- Check that user data loaded successfully (check logs for "Loaded N users from file")
- Verify dropdown populated (should show user list in left pane)

### Keyboard doesn't appear
- Ensure touch controller is properly initialized and calibrated
- Check that textarea receives CLICKED events (add debug logging)
- Verify keyboard object was created successfully in `user_mgmt_ui_create()`
- Check LVGL log output for keyboard-related messages

### Keyboard covers input fields
- Current implementation uses fixed 40% screen height
- For smaller displays, reduce keyboard height in `user_mgmt_ui_create()`:
  ```c
  lv_obj_set_size(s_keyboard, LV_PCT(100), 200);  // Fixed height instead
  ```
- Consider scrolling the form content when keyboard appears

### Cannot edit existing users
- Click on the dropdown to select a user first
- Save button only enables after user selection
- Check that text fields are clickable (not disabled)
- Verify keyboard appears when clicking on Username or PIN fields

### File corruption after power loss
- Current implementation overwrites file directly (no atomic writes)
- Backup file `users_bak.jsn` created if corruption detected on load
- Recommendation: Implement external backup mechanism for critical deployments
- Consider UPS or battery backup for hardware

### Lambda/C++ compilation errors
- ESP-IDF uses C compiler, not C++
- Do not use lambda syntax `[](){ }` in event callbacks
- Use proper C function pointers with `static` functions

## Integration Checklist

✓ **Initialization**
- [ ] SD card initialized and mounted at `/sdcard`
- [ ] `user_store_init(NULL)` called in `app_main()`
- [ ] `ui_init()` called before showing screens

✓ **Navigation**
- [ ] Button/menu item added to navigate to user management
- [ ] Admin-only access control implemented
- [ ] Return navigation configured in `user_mgmt_ui_close()`

✓ **User Interaction**
- [ ] Touch controller calibrated
- [ ] Keyboard appears on text field clicks
- [ ] Form validation working (check error messages)
- [ ] Save/Delete operations persist to file

✓ **Testing**
- [ ] Add new user and verify persistence after reboot
- [ ] Edit existing user and verify changes saved
- [ ] Delete user and verify removal from file
- [ ] Test with maximum users (64) for performance
- [ ] Verify default admin created on first run

## Performance Optimization Tips

1. **Memory Management**
   - User list allocated on heap (suitable for PSRAM)
   - Free list with `user_store_free()` when screen closed
   - Each user ~110 bytes, total ~7KB for 64 users

2. **File I/O Optimization**
   - SD card only enabled during load/save operations
   - Full list read/write (no delta updates)
   - Consider caching user list if accessing frequently

3. **UI Responsiveness**
   - Dropdown population: ~10ms for 64 users
   - Form updates: <10ms
   - File save blocks briefly (~100ms)
   - Consider progress indicator for save operations in production

4. **Touch Interaction**
   - Keyboard popup should be immediate (<50ms)
   - If laggy, check LVGL task priority in menuconfig
   - Ensure adequate CPU allocation for LVGL task
