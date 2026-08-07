Added User Store JSON Information:

Updated User Management Specification – Secure Locked Box System [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

***

## 1. Scope and Goals

This document specifies the **user** management functionality for a secure locked box system running on an ESP32-S3 with an 800×480 Waveshare ESP32-S3-Touch-LCD-4.3 module, developed in C using ESP-IDF 5.1.6 and LVGL 9.3.0. User records are stored on an SD card in a JSON file that conforms to the provided JSON Schema and are manipulated using cJSON. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

Goals:

- Provide add, delete, and edit operations for user records.
- Restrict modification of FINGERID, FACEID, and LAST_LOGON to non-UI subsystems.
- Fit within constrained flash/RAM on the ESP32-S3.
- Prefer a single LVGL-based administration screen that dynamically adapts to the operation where feasible.

***

## 2. Data Model and Storage

### 2.1 JSON Container Structure

The JSON file is a single object with a `users` array property. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

Top-level structure:

```json
{
  "users": [
    { /* user object 1 */ },
    { /* user object 2 */ }
  ]
}
```

- Type: object with required property `users`. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- `users`: array of user objects; each item is an object with `additionalProperties: false` in the schema. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  (Implementation must be tolerant to minor schema evolution but should not emit unknown fields unless explicitly supported.)

### 2.2 User JSON Schema

Each user object has the following schema (from `json_user_schema.md`). [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

Required fields:

- USERID  
  - Type: integer. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Semantics: unique system-wide user identifier.

- PIN  
  - Type: string. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Pattern: `^[0-9]{4,8}$` (4–8 digits). [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Semantics: user’s PIN code used for authentication.

Optional fields:

- FINGERID  
  - Type: integer or null. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Semantics: fingerprint template ID if registered, null if not enrolled.

- FACEID  
  - Type: integer or null. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Semantics: facial recognition template ID if registered, null if not enrolled.

- USERNAME  
  - Type: string, max length 32. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Semantics: user’s display name, must be unique in practice.

- ADMIN  
  - Type: boolean, default false. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Semantics: administrative access flag.

- LAST_LOGON  
  - Type: string or null. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Format: `date-time` (ISO 8601, UTC recommended, e.g. `"2026-01-18T13:00:00Z"`). [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Semantics: timestamp of last logon or null if never logged in. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

Notes:

- `null` for FINGERID or FACEID indicates that the corresponding biometric is not enrolled, simplifying parsing on ESP32-S3. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- LAST_LOGON uses ISO 8601 UTC to simplify later features such as inactivity lockout or audit logs. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

### 2.3 Storage Constraints and Behavior

- File location: configurable constant, e.g. `/sdcard/users.json`.
- File structure: must conform to the schema (object with `users` array). [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- `required: ["users"]` at top level; within each user object, at least USERID and PIN must be present. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- `additionalProperties: false` in the schema means unknown fields are not expected; the implementation should:
  - Avoid emitting extra fields by default.
  - If future fields are introduced, update both schema and implementation consistently.

Maximum users:

- Implementation-defined constant (e.g. 64 or 128) enforcing an upper bound to manage memory.

Error handling:

- If file missing:
  - Create a new object with `users` array containing a default admin user (schema-compliant: includes USERID and PIN, and ADMIN = true).
- If JSON invalid or schema-violating (e.g. missing `users` or non-array type):
  - Log error, move invalid file aside if possible, and create a new default JSON instance per schema.

***

## 3. Functional Requirements

### 3.1 General Behavior

- All user management operations are available only to logged-in ADMIN users (ADMIN = true). [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- The UI must not allow editing of:
  - FINGERID.
  - FACEID.
  - LAST_LOGON.
- These fields are populated by:
  - Fingerprint enrollment subsystem (FINGERID).
  - Face recognition enrollment subsystem (FACEID).
  - Authentication subsystem on successful login (LAST_LOGON as ISO 8601 string or null). [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- Changes must be written back into the `users` array and persisted to SD card.

### 3.2 Add User

Requirements:

- The admin can initiate "Add User" from the management screen.
- Fields controlled in Add flow:
  - USERNAME (optional in schema but required by application policy).
  - PIN (string) and PIN confirmation.
  - ADMIN flag.
- USERID:
  - Auto-assigned unique integer (e.g. max existing USERID + 1).
- FINGERID, FACEID:
  - Must be initialized to `null` (no biometric enrolled) when user is created. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- LAST_LOGON:
  - Must be initialized to `null` (never logged in). [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

Validation:

- USERNAME:
  - Non-empty, trimmed, length ≤ 32 characters. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Unique across all users (case-insensitive).
- PIN:
  - Must comply with schema pattern: only digits, length 4–8. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Confirmation must match.
- ADMIN:
  - Boolean; default false if not explicitly set. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- USERID:
  - Must not collide with existing users.

On success:

- New user object is appended to the `users` array in memory and persisted.
- Dropdown/list is refreshed to include the new USERNAME.

On failure:

- In case of validation or save error, display a clear error and keep in-memory list consistent.

### 3.3 Edit User

Requirements:

- Admin selects an existing user via:
  - USERNAME dropdown.
  - Search by USERID or USERNAME.
- Editable fields:
  - USERNAME.
  - PIN (string) and confirmation.
  - ADMIN flag.
- Read-only fields:
  - USERID (integer).
  - FINGERID (integer or null).
  - FACEID (integer or null).
  - LAST_LOGON (string or null).

Validation:

- USERNAME changes must preserve uniqueness and length ≤ 32. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- PIN changes must still match `^[0-9]{4,8}$` and confirmation. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- ADMIN changes must respect system rules (e.g. cannot demote the last admin or the current admin user).

On save:

- Apply changes to the selected user object in `users` array.
- Persist the full JSON object (top-level with `users` array) to SD card.

### 3.4 Delete User

Requirements:

- Admin can select a user and trigger "Delete User".
- Constraints:
  - Cannot delete the last remaining ADMIN user.
  - Cannot delete the currently logged-in ADMIN user.
- Confirmation dialog must be displayed before deletion.

Behavior:

- On confirm and after checks, remove the user object from `users` array and save.
- If save fails, revert in-memory deletion and show error.

### 3.5 Search and Selection

- Search by USERID (integer) and USERNAME (string).
- USERNAME dropdown is sourced from the `users` array.
- The UI always operates on the currently selected user in the array.

***

## 4. Non-Functional Requirements

Key points as before, with schema-aware details:

- Parsing:
  - Treat top-level as object and access `users` property as the array. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- Memory:
  - Represent PIN as a C string, not integer, to preserve leading zeros and comply with schema. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- Robustness:
  - When reading, validate types (e.g. FINGERID is integer or null, LAST_LOGON is string or null). [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

***

## 5. UI/UX Specification (LVGL 9.3.0)

Updated field types:

- PIN fields should be text/password fields accepting only digits and enforcing 4–8 character length per schema. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- LAST_LOGON should be displayed in ISO 8601 form or as a friendly placeholder such as "Never" when JSON value is null. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- FINGERID and FACEID should display "Not enrolled" when null. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

Otherwise, the single-screen layout and flows (Add, Edit, Delete, Search) remain as previously specified.

***

## 6. API and Module Design

### 6.1 Data Structures

Align C structures with schema types. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

Example:

```c
typedef struct {
    int userid;                 // USERID (required)
    int fingerid;               // FINGERID; use sentinel (e.g. -1) when JSON is null
    int faceid;                 // FACEID; same sentinel pattern
    char pin[9];                // PIN as string, 4–8 digits plus null terminator
    char username[33];          // USERNAME, up to 32 chars plus null terminator
    bool admin;                 // ADMIN
    char last_logon[32];        // LAST_LOGON ISO 8601 or empty string for null
    bool has_last_logon;        // track null vs string
    bool has_fingerid;          // track null vs value
    bool has_faceid;            // track null vs value
} user_t;
```

- When serializing:
  - If `has_fingerid` is false, emit JSON null; otherwise emit integer. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - If `has_faceid` is false, emit JSON null. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - If `has_last_logon` is false, emit JSON null; otherwise emit string. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

### 6.2 JSON Handling

- On load:
  - Parse root, retrieve `users` array, then iterate items. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
  - Validate presence of USERID and PIN for each user. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)
- On save:
  - Build root object with `users` array and append each user object.
  - Enforce `additionalProperties: false` by only adding schema-defined fields. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

Other service and UI APIs remain as described earlier but must use the updated `user_t` representation.

***

## 7. Example JSON Instance

The implementation should accept and produce JSON compatible with the example from `json_user_schema.md`. [ppl-ai-file-upload.s3.amazonaws](https://ppl-ai-file-upload.s3.amazonaws.com/web/direct-files/attachments/9673206/b5b76ef0-a50b-4079-bb8a-7081f5291fec/json_user_schema.md)

Example:

```json
{
  "users": [
    {
      "USERID": 1,
      "FINGERID": 10,
      "FACEID": 5,
      "PIN": "1234",
      "USERNAME": "Alice",
      "ADMIN": true,
      "LAST_LOGON": "2026-01-18T13:00:00Z"
    },
    {
      "USERID": 2,
      "FINGERID": 11,
      "FACEID": null,
      "PIN": "7654",
      "USERNAME": "Bob",
      "ADMIN": false,
      "LAST_LOGON": "2026-01-17T22:15:30Z"
    },
    {
      "USERID": 3,
      "FINGERID": null,
      "FACEID": 8,
      "PIN": "5555",
      "USERNAME": "Guest User",
      "ADMIN": false,
      "LAST_LOGON": null
    }
  ]
}
```


This updated specification now aligns with the formal JSON Schema and example instance in `json_user_schema.md` while preserving the original functional and UI design goals.