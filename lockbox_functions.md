# Secure Lockbox Functions

## PIN Number
- Accept 4 digit PIN input from user
- Match user's input PIN to PIN stored in user database for that user
- Allow user 3 failed tries before 5 minute lock out
- If PIN matches, go to Lock Box routine


## Facial Recognition
- Check that Facial Recognition module is unstowed
- Find Facial match
- Allow 3 user failures before failed status
- Register Face to user
- Delete Face


## Fingerprint Scan
- Register fingerprint to user
- Scan fingerprint
- Match fingerprint to user
- Delete fingerprint
- Allow 3 failed tries before failure status


## Admin Tasks
- If "current user" is "admin"
- Edit User
- Add User
- Delete User


## Lock Secure Lockbox
- User selects "Lock Box"
- If "Locked Status" is "Unlocked"
- If "Lid Status" is "Closed"
- Set "Servo Position" to "Locked" position
- Notify user box is "Locked"
- Notify user to stow Facial Recognition module


## Unlock Secure Lockbox
- Authenticate Using Fingerprint scan or Facial Recognition
- Match fingerprint or face match a user this ID's current user
- Current User enters PIN
- If PIN matches corrent user
- If "LID Status" is "Closed"
- if "Lock Status" is "Locked"
- Set "Servo Position" to "Unlocked" Position
- Set "Locked Status" to "Unlocked"
- Notify User that box is unlocked
- Prompt user to raise LID.


## Enter Sleep Mode
- If "Lid Status" is "Closed"
- If Current User is logged out
- If Facial Recognition Module is stowed
- If "Locked" status is "locked"
- Wait 1 minute
- Turn off display backlight
- Enter sleep mode


## Exit Sleep Mode
- If fingerprint touch GPIO goes high exit sleep mode
- Turn on LCD Backlight
- Display Start Screen


## Status Checks
- Check Battery Status
- Check Lid Closed Status
- Check "Locked" status
- Check Facial Recognition Module Stowed/Unstowed Status
- Display Battery Status
- Display Lid Closed Status
- Display Locked status
- Display Current User name

## Initialization Processes
- Initialize LVGL/screen
- Initialize SD Card
- Initialize "Status" structure
- "Status" structure:
    - lid_status
    - locked_status
    - face_rec_mod_status
    - battery_status
    - current_user
    - servo_position

