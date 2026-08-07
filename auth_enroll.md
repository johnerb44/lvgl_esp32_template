# Secure Lockbox — Authentication & Enrollment Spec

## Authentication Flow

- Authentication requires biometrics first, then PIN.
- Biometrics (fingerprint or face scan) identify the specific user (1:N match). PIN is then checked against that user's record only.
- Only one user may be authenticated at a time. That user is the **Current User**.
- Current User holds, at minimum: name, user_id, and admin status.
- When no user is authenticated, Current User is NULL.

### Why Biometrics First?
Biometrics uniquely identify *who* is authenticating before the PIN is checked. This prevents an attacker who knows a PIN from being able to target a specific user, and prevents the system from leaking how many users share a given PIN.

### Logoff and Sleep

- When Current User initiates logoff, the system displays a prominent screen instructing the user to:
  1. Close the Secure Lockbox lid.
  2. Stow the Face Recognition module (HLK-TX510).
- Logoff is **not complete** until both conditions are met.
  - **Lid closed**: detected via the LID Status GPIO signal.
  - **HLK-TX510 stowed**: detected by polling — if the module does not respond to a known command, it is considered stowed. If it responds, it is not stowed.
- 1 minute after logoff is confirmed, the system enters sleep mode to reduce power consumption.

---

## Roles

The system has two roles: **Admin** and **User**.

| Permission | User | Admin |
|---|---|---|
| Authenticate (biometric + PIN) | ✓ | ✓ |
| Change own PIN | ✓ | ✓ |
| Enroll own biometrics | ✓ | ✓ |
| Add / Modify / Delete users | — | ✓ |

---

## Enrollment Flow

### Admin creates the account
1. Admin logs in (biometric + PIN) and navigates to the admin interface.
2. Admin creates a new user profile with a **temporary admin-assigned PIN**.
3. Admin assigns a role (Admin or User) and notifies the new user of their temporary PIN.

### New user enrolls biometrics
4. New user accesses the **"First-Time Setup"** button on the **main screen** (not the home screen — the main screen is accessible before any login).
5. System prompts for PIN authentication (PIN-only — no biometrics enrolled yet).
6. Once PIN is accepted, the Enrollment screen opens for that user.
7. If user's FINGERID field is NOT Null then fingerprint enrollment is not allowed 
8. User scans their **fingerprint** (one finger per user — single `fingerid` field in user record). **Required.**
9. If user's FACEID field is NOT Null then face enrollment is not allowed 
10. User registers their **face** (one face per user — single `faceid` field in user record). **Optional.**
11. System saves `fingerid` and/or `faceid` to the user record.

### New user sets personal PIN
10. After biometric enrollment, the system prompts the user to change their PIN from the temporary one.
11. PIN change is **required** — the enrollment is not considered complete until the PIN is changed.
12. New PIN must be exactly 4 digits.

> **Note:** A "verify and activate" test authentication after enrollment is desirable but deferred to a future phase to keep initial implementation manageable.

---

## Implementation Scope (Phase 6)

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

## Open Questions (resolved)

| Question | Decision |
|---|---|
| Multiple fingers per user? | No — one finger per user for simplicity. |
| Multiple face angles? | No — one face per user for simplicity. |
| Is PIN change required after enrollment? | Yes — user must change the temp PIN. |
| Verify-and-activate test after enrollment? | Deferred to a future phase. |
| Admin fallback access? | Not implemented (out of scope). |