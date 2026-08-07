# User Enrollment Process

## Feature: New User Creation

### Background:
    - Given the secure lockbox is powered on
	- And New User does NOT exists in the user database with Name = "ADMIN"
	- And an admin user exists in the user database with Name = "ADMIN"
	- And the new user is named "New User" 
	- And the ADMIN has successfully authenticated using face or fingerprint
    - And The ADMIN has been successfully athenticated by PIN
    - And ADMIN is the "Current_User"
    - And ADMIN is on the Home screen

### Scenario: ADMIN Creates New User Account
    - When ADMIN selects "4 Admin" button
    - Then system goes to User Management screen
    - When Admin selects "Add New" button
    - Then Admin enters new user's USERNAME
    - Then Admin enters new user's PIN
    - Then Admin if new user is not an admin then Admin does not select Admin button
    - Then Admin presses "Save" to save new user information to the user database
    - Then the Admin selects "Back" to go back to the Home screen
    - Then the Admin notifies the new user of their PIN number


## Feature: New User Fingerprint Enrollment

### Background:
    - Given: - Given the secure lockbox is powered on
	- And the Admin has created the new user's account
	- And the Admin has told the new user what their PIN number is
	- And there exists a user in the user database with Name = "New User"
	- And the user is at the Main screen
	

### Scenario: New User Enrolls Fingerprint
    
	
    - When the new user selects "First-Time Setup" button
	- Then the system goes to the Get PIN (Enter PIN) screen
	- When the new user enters their PIN
	- And the new user presses the "Submit" button
	- Then the system matches the PIN to a valid user
	- Question: Are duplicate PINs allowed in the system???
	- Answer: Yes multiple users may have the same PIN
	- Question: How does the system validate that submitted PIN belongs to the New User???
	- When the system verifies a valid PIN
	- Then the system goes to the Biometric Enrollment screen 
	- Then the system displays message "Enrolling: New User"
	- Then the system displays 3 buttons: "Enroll Fingerprint", "Enroll Face (Optional)", "Done"
	- When the New User selects "Enroll Fingerprint"
	- Then the system checks that New User's FINGERPRINTID field is NULL
	- When the FINGERPRINTID is NULL
	- Then the system goes to the Fingerprint Enrollment screen
	- See fingerprint_enrollment_tests.md
	- When the new User's FINGERPRINTID field is NOT NULL
	- Then the system displays message "This user already has a Fingerprint enrolled - see ADMIN"
	- Then the system stays on the Biometric Enrollment screen
	- When the user presses the "Done" button
	- Then the system clears the Current_User
	- Then the system goes to the Main screen
	

## Feature: New User Face Enrollment

### Background:
    - Given the secure lockbox is powered on
	- And the Admin has created the new user's account
	- And the Admin has told the new user what their PIN number is
	- And there exists a user in the user database with Name = "New User"
    - And the user is at the Main screen


### Scenario: New User Enrolls Face
- TODO


# Feature: New User Fingerprint Enrollment

### Background:
    - Given: - Given the secure lockbox is powered on
	- And the Admin has created the new user's account
	- And the Admin has told the new user what their PIN number is
	- And there exists a user in the user database with Name = "New User"
	- And the user is at the Main screen
	

### Scenario: New User Enrolls Fingerprint
    
	
    - When the new user selects "First-Time Setup" button
	- Then the system goes to the Get PIN (Enter PIN) screen
	- When the new user enters their PIN
	- And the new user presses the "Submit" button
	- Then the system matches the PIN to a valid user
	- Question: Are duplicate PINs allowed in the system???
	- Answer: Yes multiple users may have the same PIN
	- Question: How does the system validate that submitted PIN belongs to the New User???
	- When the system verifies a valid PIN
	- Then the system goes to the Biometric Enrollment screen 
	- Then the system displays message "Enrolling: New User"
	- Then the system displays 3 buttons: "Enroll Fingerprint", "Enroll Face (Optional)", "Done"
	- When the New User selects "Enroll Fingerprint"
	- Then the system checks that New User's FINGERPRINTID field is NULL
	- When the FINGERPRINTID is NULL
	- Then the system goes to the Fingerprint Enrollment screen
	- See fingerprint_enrollment_tests.md
	- When the new User's FINGERPRINTID field is NOT NULL
	- Then the system displays message "This user already has a Fingerprint enrolled - see ADMIN"
	- Then the system stays on the Biometric Enrollment screen
	- When the user presses the "Done" button
	- Then the system clears the Current_User
	- Then the system goes to the Main screen
	

## Feature: New User Face Enrollment

### Background:
    - Given the secure lockbox is powered on
	- And the Admin has created the new user's account
	- And the Admin has told the new user what their PIN number is
	- And there exists a user in the user database with Name = "New User"
    - And the user is at the Main screen


### Scenario: New User Enrolls Face

### Test Procedure: Validate all screen components are present
    - When admin selects "4 Admin" button\
    - Then the system verifies that the ADMIN user has the attribute "ADMIN" = true
    - Then system goes to the User Management screen
    - When Admin selects "Add New" button
    - Then Admin enters new user's USERNAME
    - Then Admin enters new user's PIN
    - Then Admin if new user is not an admin then Admin does not select Admin button
    - Then Admin presses "Save" to save new user information to the user database
    - Then the Admin selects "Back" to go back to the Home screen
    - Then the Admin notifies the new user of their PIN number
- TODO