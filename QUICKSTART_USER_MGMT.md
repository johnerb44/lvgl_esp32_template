# User Management System - Quick Start Guide

## What Has Been Implemented

A complete user management system with:
- ✅ JSON-based user storage on SD card
- ✅ Add, edit, and delete user operations
- ✅ LVGL-based graphical interface
- ✅ Full validation and security constraints
- ✅ Integration with existing ESP32-S3 system

## File Structure

```
main/
├── user_store.h          # Data persistence API
├── user_store.c          # JSON file I/O with cJSON
├── user_service.h        # Business logic API
├── user_service.c        # Validation and constraints
├── user_mgmt_ui.h        # UI API
├── user_mgmt_ui.c        # LVGL interface implementation
├── main.c                # Modified to initialize user store
├── ui/
│   ├── ui.h              # Modified with user_mgmt screen
│   └── ui.c              # Modified to create user_mgmt screen
└── CMakeLists.txt        # Already includes new files
```

## Testing the Implementation

### Option 1: Direct Load (Immediate Testing)

Uncomment these lines in [main.c](main/main.c):
```c
// Load the initial screen
// lv_scr_load(ui_screen_main);
lv_scr_load(ui_screen_user_mgmt);
user_mgmt_ui_show();  // Load user data and populate UI
```

### Option 2: Add Navigation Button

Add a button to any existing screen (e.g., [ui_screen_main.c](main/ui/screens/ui_screen_main.c)):

```c
#include "user_mgmt_ui.h"
#include "lvgl_port.h"

// In the screen creation function:
lv_obj_t *btn = lv_button_create(parent);
lv_obj_set_size(btn, 200, 50);
lv_obj_t *label = lv_label_create(btn);
lv_label_set_text(label, "User Management");
lv_obj_center(label);

lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (lvgl_port_lock(-1)) {
            user_mgmt_ui_show();
            lvgl_port_unlock();
        }
    }
}, LV_EVENT_CLICKED, NULL);
```

## Build and Flash

```bash
# Build the project
idf.py build

# Flash to device
idf.py -p <PORT> flash monitor
```

## Default Credentials

On first run, a default admin user is created:
- **Username**: Admin
- **PIN**: 1234
- **Admin**: Yes
- **User ID**: 1

This user is saved to `/sdcard/users.jsn`

## Using the Interface

### Adding a User
1. Click "Add New" button
2. Enter username (1-32 characters, no spaces at start/end)
3. Enter PIN (4-8 digits)
4. Confirm PIN (must match)
5. Toggle "Administrator" switch if needed
6. Click "Save"

### Editing a User
1. Select user from dropdown
2. Modify username, PIN, and/or admin status
3. Click "Save" to commit changes
4. Click "Cancel" to discard changes

### Deleting a User
1. Select user from dropdown
2. Click "Delete" button
3. Confirm deletion in dialog
4. Note: Cannot delete yourself or the last admin

### Validation Errors
Error messages appear below the form fields in red text:
- "Username cannot be empty"
- "PIN must be 4-8 digits"
- "PINs do not match"
- "Username already exists"
- "User list is full"
- "Cannot delete this user"

## Data Persistence

### File Location
`/sdcard/users.jsn` (8.3 filename for FAT32 compatibility)

### File Format
```json
{
  "users": [
    {
      "USERID": 1,
      "FINGERID": null,
      "FACEID": null,
      "PIN": "1234",
      "USERNAME": "Admin",
      "ADMIN": true,
      "LAST_LOGON": null
    }
  ]
}
```

### Automatic Recovery
- Missing file → Creates default with admin user
- Corrupted JSON → Backs up to `users.jsn.bad` and creates new default
- Write failure → Shows error, preserves in-memory data

## System Requirements

### Hardware
- ✅ Waveshare ESP32-S3-Touch-LCD-4.3 module
- ✅ SD card inserted (FAT32 formatted)
- ✅ Touch screen for interaction

### Software
- ✅ ESP-IDF 5.1.6
- ✅ LVGL 9.3.0 (via managed components)
- ✅ cJSON (included in main/cJSON/)

## Integration with Existing Features

### Biometric Enrollment
To update fingerprint/face ID after enrollment:

```c
#include "user_store.h"
#include "user_service.h"

// After successful fingerprint enrollment:
user_list_t list;
user_store_load(&list);

int idx = user_service_find_by_userid(&list, user_id);
if (idx >= 0) {
    list.items[idx].fingerid = enrolled_fingerprint_id;
    user_store_save(&list);
}

user_store_free(&list);
```

### Login Timestamp
To update last logon after successful authentication:

```c
#include "user_store.h"
#include "user_service.h"

// After successful login:
user_list_t list;
user_store_load(&list);

int idx = user_service_find_by_userid(&list, authenticated_user_id);
if (idx >= 0) {
    // Set ISO 8601 timestamp (UTC recommended)
    snprintf(list.items[idx].last_logon, 
             sizeof(list.items[idx].last_logon),
             "2026-02-18T14:30:00Z");
    user_store_save(&list);
}

user_store_free(&list);
```

## Configuration

To modify maximum users or file path, edit [user_store.h](main/user_store.h):

```c
#define USER_STORE_MAX_USERS 64              // Change max users
#define USER_STORE_DEFAULT_PATH "/sdcard/users.jsn"  // Change file path
```

## Troubleshooting

### "Failed to open file" error
- Check SD card is inserted and initialized
- Verify `/sdcard` mount point exists
- Check file permissions

### "User list is full" error
- Increase `USER_STORE_MAX_USERS` in user_store.h
- Delete unused users to free space

### Screen appears blank
- Ensure `user_mgmt_ui_show()` is called after loading screen
- Check ESP logs for detailed error messages

### Touch not responding
- Verify touch controller is initialized
- Check I2C connection to GT911

### Changes not persisting
- Check SD card is writable (not write-protected)
- Verify sufficient free space on SD card
- Check ESP logs for write errors

## API Documentation

### Quick Reference

**Initialize**:
```c
user_store_init(NULL);  // Use default path
```

**Show Screen**:
```c
user_mgmt_ui_show();  // Loads data and displays screen
```

**Find User**:
```c
int idx = user_service_find_by_username(&list, "Admin");
int idx = user_service_find_by_userid(&list, 1);
```

**Add User**:
```c
user_t new_user = {
    .username = "Alice",
    .pin = "5678",
    .admin = false
};
user_service_add(&list, &new_user, NULL);
user_store_save(&list);
```

**Update User**:
```c
user_t updated = list.items[idx];
strcpy(updated.pin, "9999");
user_service_update(&list, idx, &updated, current_admin_id);
user_store_save(&list);
```

**Delete User**:
```c
user_service_delete(&list, idx, current_admin_id);
user_store_save(&list);
```

## Next Steps

1. **Test the interface**: Build and flash, verify basic operations
2. **Add navigation**: Add button from your main menu
3. **Integrate authentication**: Update last_logon after login
4. **Connect biometrics**: Update fingerid/faceid after enrollment
5. **Customize UI**: Adjust colors, fonts, layout as needed

## Documentation

- **Implementation Details**: See [USER_MGMT_IMPLEMENTATION.md](USER_MGMT_IMPLEMENTATION.md)
- **Integration Guide**: See [USER_MGMT_INTEGRATION.md](USER_MGMT_INTEGRATION.md)
- **Original Specification**: See [user_mgmt_spec.md](user_mgmt_spec.md)
- **JSON Schema**: See [json_user_schema.md](json_user_schema.md)

## Support

Check ESP logs for detailed error messages:
```bash
idf.py -p <PORT> monitor
```

Look for tags:
- `USER_STORE` - File I/O operations
- `USER_SERVICE` - Business logic validation
- `USER_MGMT_UI` - UI operations

## Success Criteria

✅ User management screen loads successfully
✅ Can add new users with validation
✅ Can edit existing users
✅ Can delete users (with constraints)
✅ Data persists across reboots
✅ Default admin user created on first run
✅ Touch interaction is responsive
✅ Error messages display correctly

---

**Implementation Status**: ✅ Complete and ready for testing

**Last Updated**: February 18, 2026
