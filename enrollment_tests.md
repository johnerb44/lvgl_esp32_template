# 1. Secure Lockbox — Enrollment Tests

## 1.1. Authentication Flow

- Authentication requires biometrics first, then PIN.
- Biometrics (fingerprint or face scan) identify the specific user (1:N match). PIN is then checked against that user's record only.
- Only one user may be authenticated at a time. That user is the **Current User**.
- Current User holds, at minimum: USERNAME, USERID, and ADMIN.
- When no user is authenticated, Current User is NULL.

### 1.1.1. Why Biometrics First?
Biometrics uniquely identify *who* is authenticating before the PIN is checked. This prevents an attacker who knows a PIN from being able to target a specific user, and prevents the system from leaking how many users share a given PIN.

### 1.1.2. Logoff and Sleep

- When Current User initiates logoff, the system displays a prominent screen instructing the user to:
- Close the Secure Lockbox lid.
- Stow the Face Recognition module (HLK-TX510).
- Logoff is **not complete** until both conditions are met.
- **Lid closed**: detected via the LID Status GPIO signal.
- **HLK-TX510 stowed**: detected by polling — if the module does not respond to a known command, it is considered stowed. If it responds, it is not stowed.
- 1 minute after logoff is confirmed, the system enters sleep mode to reduce power consumption.

---

## 1.2. Roles

The system has two roles: **Admin** and **User**.

| Permission | User | Admin |
|---|---|---|
| Authenticate (biometric + PIN) | ✓ | ✓ |
- | Change own PIN | ✓ | ✓ |
| Enroll own biometrics | ✓ | ✓ |
| Add / Modify / Delete users | — | ✓ |

---

## 1.3. Enrollment Flow

### 1.3.1. Admin creates the account
1. Admin logs in using biometric + PIN 
2. Upon successful authentication the system displays Home screen
3. Admin selects Admin button
4. Sytem displays User Management screen
5. Admin creates a new user profile with a **temporary admin-assigned PIN**.
6. Admin assigns a role (Admin or User) and notifies the new user of their temporary PIN.

### 1.3.2. New user enrolls biometrics
1. New user accesses the **"First-Time Setup"** button on the **main screen** (not the home screen — the main screen is accessible before any login).
2. System prompts for PIN authentication (PIN-only — no biometrics enrolled yet).
3. Once PIN is accepted, the Enrollment screen opens for that user.
4. If user's FINGERID field is NOT Null then fingerprint enrollment is not allowed 
5. User scans their **fingerprint** (one finger per user — single `fingerid` field in user record). **Required.**
6. If user's FACEID field is NOT Null then face enrollment is not allowed 
7. User registers their **face** (one face per user — single `faceid` field in user record). **Optional.**
8. System saves `fingerid` and/or `faceid` to the user record.

### 1.3.3. New user sets personal PIN
1. After biometric enrollment, the system prompts the user to change their PIN from the temporary one.
2. PIN change is **required** — the enrollment is not considered complete until the PIN is changed.
3. New PIN must be exactly 4 digits.

> **Note:** A "verify and activate" test authentication after enrollment is desirable but deferred to a future phase to keep initial implementation manageable.

---

## 1.4. Implementation Scope (Phase 6)

**In scope:**
- **"First-Time Setup"** button on the main screen (PIN-gated; for users with no biometrics enrolled yet).
- **Enrollment screen** accessible from both "First-Time Setup" and the home screen "Register Biometrics" button.
- Fingerprint enrollment (one per user, **required**) via UI.
- If user's FINGERID field is NOT Null then fingerprint enrollment is not allowed 
- Face enrollment (one per user, **optional**) via UI.
- If user's FACEID field is NOT Null then face enrollment is not allowed 
- PIN change screen (required after first enrollment; available from home screen).
- **Session service**: tracks Current User (userid, name, admin) from login through logoff.
- Home screen shows Current User name and lock status (LOCKED / UNLOCKED).
- "Sign Out / Lock" button: locks servo, prompts lid-close + HLK stow check, then returns to main screen.

**Deferred:**
- Multiple fingers per user.
- Multiple face angles per user.
- Post-enrollment verification test.
- Admin override / fallback access.
- Hashed PIN storage or encrypted biometric data.
- Sleep mode implementation (1-minute timer after logoff).

---

## 1.5. Open Questions (resolved)

| Question | Decision |
|---|---|
| Multiple fingers per user? | No — one finger per user for simplicity. |
| Multiple face angles? | No — one face per user for simplicity. |
| Is PIN change required after enrollment? | Yes — user must change the temp PIN. |
| Verify-and-activate test after enrollment? | Deferred to a future phase. |
| Admin fallback access? | Not implemented (out of scope). |


## Scenario: 1 User Registers Fingerprint

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
    - And the test user has been created in json user database 
	- And the test user has a valid PIN assigned by Admin 
    - And the test user's FINGERID field in the json user database is NULL

### Test Procedure
	1 When the user selects "First-Time Setup"
	2 Then the system goes to Biometric screen
    3 When the user selects "Face Scan"
	4 Then the system goes to the Face Scan screen
    5 When the user selects "Begin Scan"
    6 And the user positions an unregistered face on the hlk-tx510 display
    7 Then the system sends Identification Command (0x12) to hlk-tx510
	8 Then hlk-tx510 Identification Command returns error code 0x03 OR 0x06 OR 0x07
    9 Then the system displays message "Face scan failed - try again"
   10 Then system stays on Face Scan screen
   11 When the user selects "Back" button
   12 Then the system returns to the biometric selection screen

### Test Results
    - Step 9 fails
    - Expected: The system displays message "Face scan failed - Please try again"
    - Actual: the system displays message "Liveness check failed. Please try again."
    - Issue: issue-0003 identified 5/03/2026.
    - Issue: issue-0003 resolved 5/03/2026. Modified code to display desired message.