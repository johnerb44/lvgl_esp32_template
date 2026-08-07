# Secure Lockbox Biometric Enrollment Testing


## Scenario: Fingerprint Enrollment - New user, first time fingerprint enrollment 

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
    - And the test user has a valid user account
    - And test user's fingerprint has not been enrolled
    - And the test user has been assigned a valid PIN in the user database
    - And the test user has been notified, by the Administrator, what their new PIN is
    - And system is configured to use the R503 automatic fingerprint enrollment (AutoEnroll) command (0x31)
    - Given the user is intentionally enrolling fingerprint for the first time
  
  ### Test Procedure
    1. When the user selects "Begin authentication"
    2. Then the system goes to Biometric screen
    3. When the user selects "Enroll Fingerprint"
    4. Then the system goes to the Fingerprint Scan screen
    5. When the user selects "Enroll Fingerprint"
    6. Then the system goes to the "Enter PIN" screen
    7. Then the user enters the 4 digit PIN assigned by the Admin
    8. Then the users presses the "Submit" button
    9.  Then the system validates the entered PIN
    10. When the PIN is validated
    11. Then the system goes to the Enroll Fingerprint screen
    12. Then the system sends the AutoEnroll Command (0x31) to the R503 Fingerprint Scanner
    13. When the R503 LED flashes Blue
    14. Then the user places their finger on fingerprint scanner
    15. Then the R503 Led briefly flashes Yellow (Scan image complete)
    16. When the R503 Led flashes White
    17. Then the user removes their finger from the R503 Fingerprint Scanner
    18. When the R503 LED flashes Blue 
    19. Then the same process (steps 14-18) continues 5 more times
    20. When the users finger has been scanned 6 times
    21. Then the R503 LED flashes Green
    22. Then the R503 returns the R503 database ModelID for the user's fingerprint
    23. Then the system populates the user's json user database FINGERID field with R503's ModelID
    24. Then the system displays message "Fingerprint Enrolled"
    25. Then the system resets Current_User to NULL
    26. Then the system waits 5 seconds
    27. Then the system goes to the Main screen
 
### Test Results
    - Not Tested


## Scenario: Fingerprint Scan - Finger Found in R503 database and no Finger Match found in user database

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
    - And the test user has a valid user account
    - And test user's fingerprint has not been enrolled
    - And the test user has been assigned a valid PIN in the user database
    - And the test user has been notified, by the Administrator, what their new PIN is
    - And system is configured to use the R503 automatic fingerprint enrollment (AutoEnroll) command (0x31)
    - Given the user is intentionally enrolling fingerprint for the first time

### Test Procedure
    1. When the user selects "Begin authentication"
	2. Then the system goes to Biometric screen
    3. When the user selects "Fingerprint Scan"
	4. Then the system goes to the Fingerprint Scan screen
    5. When the user selects "Begin Scan"
    6. Then the R503 LED "breaths White light"
    7. And the user places a previously scanned finger on the fingerprint scanner
    8. When the R503 completes sucessful fingerprint image collection
    9. Then the R503 returns Step 1 packet (image collection) confirmation success code 0x00
    10. Then the R503 LED lights up Yellow
    11. Then the R503 Generates the fingerprint feature
    12. Then the R503 returns Step 2 packet (feature generation) confirmation success code 0x00
    13. Then the R503 searches and compares the image with fingerprint templates in the R503 database
    14. When the R503 finds a matching fingerprint template in the R503 fingerprint database
    15. Then the R503 returns Step 3 packet (search) confirmation success code 0x00
    16. And the R503 returned Step 3 packet contains ModelID
    17. Then the R503 LED lights up Green
    18. Then the system matches user's "FINGERID" field to R503 returned "ModelID"
    19. When the match is sucessful
    20. Then the system displays message "Fingerprint found - User match found"
	21. Then system stays on Fingerprint Scan screen for 5 seconds
	22. Then the system goes to the PIN Entry screen
  
  ### Test Results
    - Not Tested



 - Then the R503 LED "breaths White light"
    - And the user places a previously scanned finger on the fingerprint scanner
    - When the R503 completes sucessful fingerprint image collection
    - Then the R503 returns Step 1 packet (image collection) confirmation success code 0x00
    - Then the R503 LED lights up Yellow
    - Then the R503 Generates the fingerprint feature
    - Then the R503 returns Step 2 packet (feature generation) confirmation success code 0x00
    - Then the R503 searches and compares the image with fingerprint templates in the R503 database
    - When the R503 finds a matching fingerprint template in the R503 fingerprint database
    - Then the R503 returns Step 3 packet (search) confirmation success code 0x00
    - And the R503 returned Step 3 packet contains ModelID
    - Then the R503 LED lights up Green
    - Then the system matches user's "FINGERID" field to R503 returned "ModelID"
    - When the match is sucessful
    - Then the system displays message "Fingerprint found - User match found"
	- Then system stays on Fingerprint Scan screen for 5 seconds
	- Then the system goes to the PIN Entry screen