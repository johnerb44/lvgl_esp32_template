User Management Specification – Secure Locked Box System [docs.lvgl](https://docs.lvgl.io/9.4/details/integration/chip_vendors/espressif/tips_and_tricks.html)

***

## 1. Scope and Goals

This document specifies the **user** management functionality for a secure locked box system running on an ESP32-S3 with an 800×480 Waveshare ESP32-S3-Touch-LCD-4.3 module, developed in C using ESP-IDF 5.1.6 and LVGL 9.3.0 User records are stored in a JSON file on SD card and manipulated using cJSON. [waveshare](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3)

Goals:

- Provide add, delete, and edit operations for user records.
- Restrict modification of FINGERID, FACEID, and LASTLOGON to non-UI subsystems.
- Fit within constrained flash/RAM, leveraging PSRAM as appropriate on the ESP32-S3 board. [waveshare](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3)
- Present a single LVGL-based administration screen that dynamically adapts to the operation where feasible.

***

## 2. Data Model and Storage

### 2.1 User JSON Schema

Each user is represented as a JSON object within a top-level array stored in a single file on the SD card (e.g. `/sdcard/users.jsn`). [components.espressif](https://components.espressif.com/components/espressif/cjson)

The JSON file is a single object with a `users` array property. json_user_schema.md

Top-level structure:

```json
{
  "users": [
    { /* user object 1 */ },
    { /* user object 2 */ }
  ]
}
```

- Type: object with required property `users`. json_user_schema.md
- `users`: array of user objects; each item is an object with `additionalProperties: false` in the schema. json_user_schema.md

(Implementation must be tolerant to minor schema evolution but should not emit unknown fields unless explicitly supported.)


### 2.2 Storage Constraints and Behavior

- File location: configurable constant, default `/sdcard/users.jsn`.
- Note: SD fat32 system can handle file names in 8.3 file convention only.
- File format: UTF-8 encoded JSON, minified or pretty-printed at implementation discretion.
- Maximum users: 25 configurable constant (e.g. 64 or 128) chosen to respect available memory and UI usability; out-of-range behavior must be defined (reject new user creation with error).
- On load failure (file missing, corrupted JSON):
  - If missing: create a default file with a single built-in admin account (implementation-defined defaults) and force admin to change PIN on first use.
  - If corrupted: log error, move bad file to a backup name (e.g. `users_corrupt.jsn`) if possible, create a new default file as above.

***

Required fields:

- USERID  
  - Type: integer. (from json_user_schema.md)
  - Semantics: unique system-wide user identifier.

- PIN  
  - Type: string. (from json_user_schema.md)
  - Pattern: `^[0-9]{4,8}$` (4–8 digits). (from json_user_schema.md)
  - Semantics: user’s PIN code used for authentication.

Optional fields:

- FINGERID  
  - Type: integer or null. (from json_user_schema.md)
  - Semantics: fingerprint template ID if registered, null if not enrolled.

- FACEID  
  - Type: integer or null. (from json_user_schema.md)
  - Semantics: facial recognition template ID if registered, null if not enrolled.

- USERNAME  
  - Type: string, max length 32. (from json_user_schema.md)
  - Semantics: user’s display name, must be unique in practice.

- ADMIN  
  - Type: boolean, default false. (from json_user_schema.md)
  - Semantics: administrative access flag.

- LAST_LOGON  
  - Type: string or null. (from json_user_schema.md)
  - Format: `date-time` (ISO 8601, UTC recommended, e.g. `"2026-01-18T13:00:00Z"`). (from json_user_schema.md)
  - Semantics: timestamp of last logon or null if never logged in. (from json_user_schema.md)

Notes:

- `null` for FINGERID or FACEID indicates that the corresponding biometric is not enrolled, simplifying parsing on ESP32-S3. (from json_user_schema.md)
- LAST_LOGON uses ISO 8601 UTC to simplify later features such as inactivity lockout or audit logs.   (from json_user_schema.md)

### 2.3 Storage Constraints and Behavior

- File location: configurable constant, e.g. `/sdcard/users.jsn`.
- File structure: must conform to the schema (object with `users` array). (from json_user_schema.md)
- `required: ["users"]` at top level; within each user object, at least USERID and PIN must be present. (from json_user_schema.md)
- `additionalProperties: false` in the schema means unknown fields are not expected; the implementation should:
  - Avoid emitting extra fields by default.
  - If future fields are introduced, update both schema and implementation consistently.

Maximum users: 

- 25 Implementation-defined constant (e.g. 64 or 128) enforcing an upper bound to manage memory.

Error handling:

- If file missing:
  - Create a new object with `users` array containing a default admin user (schema-compliant: includes USERID and PIN, and ADMIN = true).
- If JSON invalid or schema-violating (e.g. missing `users` or non-array type):
  - Log error, move invalid file aside if possible, and create a new default JSON instance per schema.

***

See: json_user_schema.md

Fields (current):

- USERID: integer, unique, positive, required.
- FINGERID: integer, unique per fingerprint template, optional or null if not enrolled.
- FACEID: integer, unique per face template, optional or null if not enrolled.
- PIN: integer, 4–8 digits, required.
- USERNAME: string, unique, case-insensitive comparison for lookups, required.
- ADMIN: boolean, true for administrator accounts, required.
- LASTLOGON: string, ISO-8601 formatted timestamp (e.g. `"2026-02-18T17:10:00Z"`), required.

Additional fields:

- The JSON object may include 1–2 extra fields in the future (e.g. "EMAIL", "NOTES", "PINCHANGEREQUIRED"); the parser must ignore unknown fields but preserve them when saving (read-modify-write strategy). [components.espressif](https://components.espressif.com/components/espressif/cjson)


## 3. Functional Requirements

### 3.1 General Behavior

- All user management operations are available only to logged-in ADMIN users.
- Currently logged-in admin must not be able to delete or demote themselves (to prevent lockout).
- FINGERID, FACEID, and LASTLOGON must be treated as read-only from the user management UI.
  - FINGERID and FACEID are set exclusively by the enrollment subsystems.
  - LASTLOGON is maintained by the authentication subsystem when a user successfully logs in.
- Changes must be persisted to the JSON file on SD card, with atomic write semantics where practical:
  - Write to a temporary file.
  - Flush and fsync.
  - Rename over original.
- All operations must validate inputs at both UI and business-logic layers.

### 3.2 Add User

Requirements:

- The admin can initiate "Add User" via a button on the user management screen.
- The add flow allows the admin to define:
  - USERNAME.
  - PIN (with confirmation).
  - ADMIN flag.
- USERID is auto-assigned:
  - Implementation: next available positive integer not currently used (e.g. max existing USERID + 1, or smallest unused ID).
- FINGERID, FACEID: initialized to sentinel values (e.g. -1) to indicate no enrollment yet.
- LASTLOGON: initialized to an empty string or a sentinel (e.g. `""`).
- Validation:
  - USERNAME non-empty, no leading/trailing spaces, enforce a max length (e.g. 32 characters).
  - USERNAME must be unique (case-insensitive).
  - PIN must be numeric, 4–8 digits, and PIN and confirmation must match.
- On success:
  - New user is appended to in-memory list and written to JSON file.
  - UI updates dropdown/user list to include new user.
- On failure (e.g. file write error, capacity exceeded):
  - Show error message and leave in-memory list unchanged.

### 3.3 Edit User

Requirements:

- Admin can select an existing user via:
  - Search by USERID or USERNAME (text input).
  - USERNAME dropdown list populated from in-memory users.
- On selection, the editable fields presented:
  - USERNAME.
  - PIN (with confirmation; may allow leaving PIN blank to keep unchanged).
  - ADMIN flag.
- Non-editable fields must be visible but read-only:
  - USERID. (System Assigned)
  - FINGERID. (Admin must have ability to reset FINGERID to NULL)
  - FACEID.   (Admin must have ability to reset FACEID to NULL)
  - LASTLOGON.
- Validation:
  - If USERNAME changed: enforce uniqueness and rules as in Add User.
  - If PIN changed: numeric, 4–8 digits, and confirmation match.
  - Admin cannot uncheck their own ADMIN flag.
  - Admin cannot delete Account with username "ADMIN"
  - There cannot be duplicate instances of "USERNAME"
  - There cannot be duplicate instances of "USERID"
- On save:
  - Update the in-memory object.
  - Persist to JSON file via atomic write.
  - Refresh dropdown/list and any displayed details.
- On cancel:
  - Discard unsaved changes and restore view of last saved data.

### 3.4 Delete User

Requirements:

- Admin can select a user (as in Edit) and invoke "Delete User".
- Safety constraints:
  - Prevent deletion of:
    - The last remaining ADMIN user.
    - The currently logged-in admin user.
  - Present a confirmation dialog: "Delete user '<USERNAME>' (ID <USERID>)? This action cannot be undone."
- Behavior:
  - On confirmation: remove user from in-memory list, write updated list to JSON.
  - On successful write: update UI dropdown/list to remove user.
  - On write failure: display error and restore in-memory list from previous state.

### 3.5 Search and Selection

Requirements:

- Provide search by:
  - USERID (numeric input).
  - USERNAME (string input, case-insensitive, with partial match or prefix match, implementation-defined).
- Provide a USERNAME dropdown:
  - Populated from in-memory list.
  - Sorted alphabetically.
- Selecting from dropdown or search must:
  - Load the user details into form fields.
  - Visually indicate which user is currently active.

***

## 4. Non-Functional Requirements

### 4.1 Performance and Memory

- The full user list is loaded into memory at startup or on first access to user management.
- For typical user counts (e.g. ≤128), store all users in RAM; leverage PSRAM on the ESP32-S3 module where appropriate to relieve internal RAM pressure. [waveshare](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3)
- JSON parsing and serialization must use cJSON API efficiently (reuse buffers where possible, free all allocations explicitly). [components.espressif](https://components.espressif.com/components/espressif/cjson)
- UI operations (list population, screen redraws) must maintain responsive touch interaction (target <100 ms perceived latency for common operations). [docs.lvgl](https://docs.lvgl.io/9.4/details/integration/chip_vendors/espressif/tips_and_tricks.html)

### 4.2 Robustness

- Handle SD card absence/unmount gracefully:
  - Detect missing card on first access.
  - Show clear error and disable write operations; allow read-only behavior if possible.
- Handle JSON errors:
  - If parsing fails, present a diagnostic message (for developer logs) and create a new default file as described.
- Ensure all pointer and index operations have bounds checks to avoid crashes in constrained embedded environment.

### 4.3 Security

- Only ADMIN users can access user management UI.
- PIN values are sensitive:
  - Do not log plaintext PINs.
  - Mask PIN entry on screen.
- Consider rate-limiting admin actions to guard against accidental rapid changes (implementation detail, optional).

***

## 5. UI/UX Specification (LVGL 9.3)

### 5.1 General Layout

Target: Single LVGL screen (`lv_obj_t *user_mgmt_screen`) with dynamic content by mode (View/Edit/Add).

Recommended layout:

- Header row:
  - Title label: "User Management".
  - Close/back button to exit admin screen.
- Left pane:
  - USERNAME dropdown (full-height or top-left region).
  - Search area:
    - Text input for USERNAME.
    - Numeric input for USERID.
    - "Search" button.
- Right pane (mode-dependent form):
  - Static labels and corresponding fields:
    - USERID: read-only label.
    - USERNAME: editable text area.
    - PIN: password field.
    - Confirm PIN: password field.
    - ADMIN: toggle switch or checkbox.
    - FINGERID: read-only label.
    - FACEID: read-only label.
    - LASTLOGON: read-only label.
- Bottom row:
  - Buttons: "Add New", "Save", "Delete", "Cancel".
  - "Save" and "Delete" enabled/disabled depending on context (e.g. Save active on Edit/Add, Delete active only when a user is selected).

Screen modes:

- View/Edit mode: shows selected user; fields mostly read-only until "Edit" state is activated (if desired).
- Add mode: user object fields cleared or set to defaults, USERID displayed as "auto" until saved.

The design should use LVGL 9.4 recommended patterns for input fields, dropdowns, and layouts on ESP32-S3 (e.g. flex or grid layouts, avoiding unnecessary widget nesting). [docs.lvgl](https://docs.lvgl.io/9.4/details/integration/chip_vendors/espressif/tips_and_tricks.html)

### 5.2 Interaction Flows

Add User:

1. Admin presses "Add New".
2. Form on right clears and switches to Add mode.
3. Admin enters USERNAME, PIN, Confirm PIN, and sets ADMIN if desired.
4. Admin presses "Save".
5. Validation:
   - Show inline error labels under fields or a modal alert if validation fails.
6. On success:
   - New user added, dropdown refreshed, screen returns to View/Edit mode on the new user.

Edit User:

1. Admin selects user from dropdown or via search.
2. Right pane loads user in View mode.
3. Admin modifies USERNAME, PIN (if changing), and ADMIN.
4. Admin presses "Save".
5. Perform validations and save or show errors as appropriate.

Delete User:

1. Admin selects a user.
2. Admin presses "Delete".
3. Confirmation dialog appears.
4. On confirm, perform safety checks and delete or show error if disallowed.

Search:

1. Admin types USERNAME or USERID.
2. On pressing "Search":
   - If exact match found: select user.
   - If multiple partial matches: show first match and optionally a small list selector.
   - If no match: show "User not found" dialog.

***

## 6. API and Module Design

### 6.1 Modules

Proposed modules:

- `user_store.c/h` – data model and persistence (JSON + cJSON).
- `user_service.c/h` – business logic (validation, constraints, search).
- `user_mgmt_ui.c/h` – LVGL screen and event handlers.

This separation allows the code assistant to generate focused C implementations per concern.

### 6.2 user_store API (Data + JSON)

Responsibilities:

- Load and save user list from/to JSON file.
- Provide basic access to in-memory array.

Key types (conceptual):

- `typedef struct { int userid; int fingerid; int faceid; int pin; char username[33]; bool admin; char lastlogon[32]; /* plus opaque/extra fields handling */ } user_t;`
- `typedef struct { user_t *items; size_t count; size_t capacity; } user_list_t;`

Key functions (signatures indicative):

- `esp_err_t user_store_init(const char *path);`
- `esp_err_t user_store_load(user_list_t *out_list);`
- `esp_err_t user_store_save(const user_list_t *list);`
- `void user_store_free(user_list_t *list);`

Implementation notes:

- Use cJSON to parse the top-level array and map fields into `user_t` structs. [components.espressif](https://components.espressif.com/components/espressif/cjson)
- For unknown JSON fields, either:
  - Store them in an opaque cJSON object attached to the user (for merge-on-save), or
  - Keep the original cJSON representation in memory and update known fields in-place before re-serializing.

### 6.3 user_service API (Business Logic)

Responsibilities:

- Validation, ID assignment, search, and operations independent of UI.

Key functions:

- `int user_service_find_by_username(const user_list_t *list, const char *username);`  
  Returns index or -1.
- `int user_service_find_by_userid(const user_list_t *list, int userid);`
- `esp_err_t user_service_add(user_list_t *list, const user_t *new_user, int *out_index);`
- `esp_err_t user_service_update(user_list_t *list, int index, const user_t *updated_user, int current_admin_userid);`
- `esp_err_t user_service_delete(user_list_t *list, int index, int current_admin_userid);`
- `bool user_service_is_unique_username(const user_list_t *list, const char *username, int ignore_index);`
- `int user_service_next_userid(const user_list_t *list);`

Rules enforced:

- Unique USERNAME (case-insensitive).
- USERID allocation policy.
- Prevent deletion of last ADMIN and of current_admin_userid.
- Prevent demotion (ADMIN -> non-ADMIN) of current_admin_userid.

### 6.4 user_mgmt_ui API (LVGL)

Responsibilities:

- Create, show, and manage the user management screen.
- Marshal data between UI widgets and `user_t` structures.

Key functions:

- `void user_mgmt_ui_create(lv_obj_t *parent, int current_admin_userid);`
- `void user_mgmt_ui_show(void);`
- `void user_mgmt_ui_close(void);`

Internals:

- Event handlers for:
  - Dropdown selection changed.
  - Search button pressed.
  - Add/New, Edit, Save, Delete, Cancel buttons. 
- For Save/Delete, call into `user_service` and `user_store` as appropriate.

***

## 7. Integration Points

### 7.1 Authentication and Enrollment

- Fingerprint enrollment module:
  - On successful enrollment, updates the selected user’s FINGERID via `user_service_update` and persists with `user_store_save`.
- Face recognition enrollment module:
  - On successful enrollment, updates FACEID similarly.
- Authentication subsystem:
  - On successful user login, updates LASTLOGON to the current time (ISO-8601 string) and saves via `user_store_save`.
- The user management UI must not provide controls to alter these fields.

### 7.2 System Configuration

- File path for JSON and max users should be configurable at build-time (Kconfig options) or compile-time macros in a config header.
- The user management module should expose basic error codes that higher-level system code can map to user-visible messages.
- The user management module uses ESP-IDF SD library and cJSON library.
- The SD Card is part of the Waveshare ESP32-S3-Touch-LCD-4.3 module and must be initialized as an SPI interface.  
- The module uses an ESP_IOExpander_CH422G to control the status of pins such as touchscreen reset (TP_RST), LCD backlight (LCD_BL), LCD reset (LCD_RST), TF card  select (SD_CS), and USB select (USB_SEL). 
- Initialization of the SD (TF) card must not conflict with these other functions.
- The ESP-IDF framework uses sdconfig to define interface PINs and other system settings. This file may have to be modified to integrate SD card pin definitions with other ESP32-S3-Touch-LCD-4.3 module interfaces.

***

## 8. Testing and Validation

- Unit tests for:
  - JSON load/save with cJSON, including unknown extra fields.
  - Add, edit, delete operations and their constraints.
  - Search functions and username uniqueness checks.
- Integration tests:
  - Verify user management screen behavior with a populated dataset near capacity.
  - Verify behavior with missing or corrupted JSON file on SD card.
- Manual UI tests on target hardware:
  - Touch interactions.
  - Performance and latency of operations on the Waveshare ESP32-S3-Touch-LCD-4.3 module. [waveshare](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3)

This specification is intended to be concrete enough for an AI code assistant to generate initial C module scaffolding and LVGL-based UI code that can be refined by a human engineer.