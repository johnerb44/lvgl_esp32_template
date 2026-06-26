# Screen Components

## User Management Screen Components
- Source Code: 
- Header: 
- Title: "User Management"
- Button: "Back"
- Dropdown List: Users
  - For each user, when selected, show:
    - USERNAME
    - USERID
    - FINGERID
    - FACEID
    - LAST_LOGON
- Text Input: Username
- Text Input: PIN
- Text Input: Confirm PIN
- Check Box: Show PIN
- Check Box: Admin
- Check Box: Unenroll Finger   **NEW
- Check Box: Unenroll Face     **NEW
- Button: "Add New"
- Button: "Save"
- Button: "Delete"
- Button: "Cancel"
- The Dropdown List IS scrollable through entire list of users in user json database
- The User Management screen should NOT be scrollable


## Main Screen Components

- Source Code: ui_screen_main.c
- Header: ui_screen_main.h
- Title: "Secure Lock Box"
- Instruction Text: "M.O.S.S. has classified the contents of this box: 
- Instruction Text:              "TOP SECRET"
- Instruction Text: "Access requires biometric and PIN authentication."
- Button: "Begin Authentication" Color: Orange (0xe19419)
- Button: "First-Time Setup" Color: Gray (0x6c757d)
- Button: "FLASH" Color: Blue
- Message: None


## Home Screen Components

## Get PIN Screen Components

## Change PIN Screen Components

## Logout Screen Components

## Biometric Screen Components

- Source Code: ui_screen_biometric.c
- Header: ui_screen_biometric.h
- Title: None
- Instruction Text: "Select a biometric method"
- Button: "Fingerprint Scan" Color: Orange (0xe99309)
- Button: "Face Scan" Color: Blue (0x19abe0)
- Button: "Back" Color: TBD  *** Need to add???
- Message: None

## Fingerprint Scan Screen Components

- Source Code: ui_screen_fpscan.c
- Header: ui_screen_fpscan.h
- Title: "Fingerprint Scan"
- Instruction Text: "1. Fingerprint Scanner Ring is WHITE"
- Instruction Text: "2. Raise Protective Cover"
- Instruction Text: "3. Place Finger on Black Area of Scanner"
- Button: "Begin Scan" Color: Yellow/Orange (0xe19419))
- Button: "Enroll FP"  Color: Orange (0xe07019)
- Button: "< Back" Color: Gray (0x444444)
- Message: "Ready to scan"
- Message: "Scanning"
- Message: "Fingerprint found - User match found"
- Message: "Fingerprint found - User match not found"
- Message: "Fingerprint not found-try another finger"
- Message: "Scan error"
- Message: "Finger not detected - place finger on scanner"

## Face Scan Screen Components

- Source Code: ui_screen_fpscan.c
- Header: ui_screen_fpscan.h
- Title: "Face Scan"
- Instruction Text: "1. Position your face in front of camera"
- Instruction Text: "2. Ensure face is well lit"
- Instruction Text: "3. Press Begin Scan"
- Button: "Begin Scan" Color: Blue (0x19abe0)
- Button: "Enroll Face"  Color: Orange (0xe07019)
- Button: "< Back" Color: Gray (0x444444)
- Message: "Face Scanner Ready"
- Message: "Face scan failed - try again"
- Message: "Face not found - try again"
- Message: "Face found - User found"
- Message: "Raise Face Scanner first"
- Message: "Scan error"
- Message: "Finger not detected - place finger on scanner"

## First Time User Screen Components

## Logout Screen Components

## Register Face Screen Components

## Register Finger Screen Components

## Enroll Screen Components
