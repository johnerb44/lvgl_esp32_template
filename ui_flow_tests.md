# UI Flow Tests


## Scenario: 1  From: User Authentication using Fingerprint scan To: Secure Lock Box Unlocked

### Background:
    - Given the secure lockbox has power
    - And the system is on the Main screen
	- And the test user's finger has already been enrolled 
    - And the test user has been created in json user database
    - And the test user is not an admin 
   
### Test Procedure
    1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Fingerprint Scan"
	4 Then the system goes to the Fingerprint Scan screen
    5 When the user selects "Begin Scan"
    6 Then the Fingerprint Scanner LED turns WHITE
    7 And the user positions finger on Fingerprint Scanner module
    8 Then the Fingerprint Scanner matches the user's to a fingerprint template in the R503 database
    9 When a match is found the Fingerprint Scanner LED turns GREEN
   10 Then the system displays message "Fingerprint found - User found"
   11 Then system stays on Fingerprint Scan screen for 4 seconds
   12 Then the system goes to the Enter PIN screen
   13 Then the PIN is displayed as "----" indicating no PIN has been entered yet
   14 When the users enters the full 4 digit PIN, the PIN is displayed as "****"
   15 When the user presses the "Backspace" key
   16 Then the last entered digit is erased and replaced on the screen as "-" (example: "***-")
   17 When the user presses the "Clear" button
   18 Then all digits entered are erased and the PIN is displayed as "----" again
   19 When the user checks the Show PIN checkbox
   20 Then the PIN shows as "1234"
   21 When the user presses the SUBMIT button
   22 And the user has entered an Invalid PIN 
   23 Then the screen displays a message "Invalid PIN (x attempts left)"
   24 When the user presses the SUBMIT button
   25 And the user has entered Valid PIN 
   26 Then the screen displays a message "PIN Accepted" for 5 seconds
   27 Then the system goes to the Home screen
   28 Then the system displays "Welcome, User" directly under the screen title
   29 Then the screen display locked status "[LOCKED]" in the upper right corner of the screen
   30 When the user selects the "1 Unlock Secure Box" button
   31 Then the system unlocks the box
   32 Then the screen displays a message "Box Unlocked" for 4 seconds
   33 Then the screen display locked status "[UNLOCKED]" in the upper right corner of the screen


### Test Results 
    - Not tested


## Scenario: 2  From: User Authentication using Fingerprint scan To: Change PIN completed

### Background:
    - Given the secure lockbox has power
    - And the system is on the Main screen
	- And the test user's finger has already been enrolled 
    - And the test user has been created in json user database
    - And the test user is not an admin 
   
### Test Procedure
    1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Fingerprint Scan"
	4 Then the system goes to the Fingerprint Scan screen
    5 When the user selects "Begin Scan"
    6 Then the Fingerprint Scanner LED turns WHITE
    7 And the user positions finger on Fingerprint Scanner module
    8 Then the Fingerprint Scanner tries to matche the user's fingerprint to a fingerprint template in the R503 database
    9 When a match is found the Fingerprint Scanner LED turns GREEN
   10 Then the system displays message "Fingerprint found - User found"
   11 Then system stays on Fingerprint Scan screen for 4 seconds
   12 Then the system goes to the Enter PIN screen
   13 Then the PIN is displayed as "----" indicating no PIN has been entered yet
   14 When the users enters the full 4 digit PIN, the PIN is displayed as "****"
   15 When the user presses the "Backspace" key
   16 Then the last entered digit is erased and replaced on the screen as "-" (example: "***-")
   17 When the user presses the "Clear" button
   18 Then all digits entered are erased and the PIN is displayed as "----" again
   19 When the user checks the Show PIN checkbox
   20 Then the PIN shows as "1234"
   21 When the user presses the SUBMIT button
   22 And the user has entered an Invalid PIN 
   23 Then the screen displays a message "Invalid PIN (x attempts left)"
   24 When the user presses the SUBMIT button
   25 And the user has entered Valid PIN 
   26 Then the screen displays a message "PIN Accepted" for 5 seconds
   27 Then the system goes to the Home screen
   28 Then the system displays "Welcome, User" directly under the screen title
   29 Then the screen display locked status "[LOCKED]" in the upper right corner of the screen
   30 When the user selects the "2 Change PIN" button
   31 Then the system goes to the "Change PIN" screen
   32 Then there is a "Change PIN" title at the top Left
   33 Then the screen displays "Enter Current PIN" at the top of the screen
   34 Then the user enters their current PIN
   35 When the user presses the "Submit" button
   36 Then the system refreshes the screen and displays "Enter New PIN" at the top center of the screen.
   37 Then there is (still) a "Change PIN" title at the top Left
   38 Then the user enters their New PIN (New PIN may be the same as the old one)
   39 When the user presses the "Submit" button
   40 Then the system displays message "PIN changed successfully!" for four seconds
   41 Then the system goes to the Home screen
   42 When the user presses the "2 Change PIN" button the Change PIN process starts again



### Test Results 
    - Tested
    - Step 32: There is no "Change PIN" title that remains on the "Change PIN" screen throughout the whole change PIN process
    - "Enter Current PIN" and "Enter New PIN" are instructions, not titles
    - If the user changes their mind and does not want the change their PIN, the Change PIN screen has no way to escape without - changing the PIN. Needs a "Back" buuton
    - Step 42: If user has changed their PIN and has been brought back to the "Home" screen when the user presses the "2 Change PIN" button, the "Change PIN" screen comes up for about 2-3 seconds then automatically goes back the "Home" screen.
    - The user should be able to change their PIN an unlimited time during the same session.


## Scenario: 3  From: User Authentication using Face Scan To: Secure Lock Box Unlocked