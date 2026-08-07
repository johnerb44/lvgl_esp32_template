# Secure lockbox face biometric authentication


## Scenario: 1  Face Scan - Face found in HLK-TX510 database and face match found in user database

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
	- And hlk-tx510 face scanner module is unstowed
	- And the test user's face has already been enrolled 
    - And face "user_id" is in the hlk-tx510 database
	- And the test user has been created in json user database 
    - And the test user's "FACEID" field is not NULL

### Test Procedure
    1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Face Scan"
	4 Then the system goes to the Face Scan screen
    5 When the user selects "Begin Scan"
    6 And the user positions face on hlk-tx510 display
	7 And the hlk-tx510 Identification Command (0x12) returns result 0x00 (success)
    8 And the hlk-tx510 Identification Command returns hlk-tx510 "user_id"
    9 And the system is able to find a registered user in the json user database that has a "FaceID" that matches returned hlk-tx510 "user_id"
   10 Then the system displays message "Face found - User found"
   11 Then system stays on Face Scan screen for 4 seconds
   12 Then the system goes to the Get PIN screen

### Test Results 
    - Step 10 failed
    - Expected: the system displays "Face found - User found"
    - Expected: system stays on Face Scan screen for 4 seconds
    - Actual: after step 9 system does not stay on Face Scan screen for 4 seconds - goes immediately to Get PIN screen
    - Issue: issue-0001 identified. 4/25/2026
    - Issue: issue-0001 resolved. 4/25/2026

  
		
## Scenario: 2 Face is found in the hlk-tx510 database but does not match the "FACEID" of any registered user in the json user database

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
	- And hlk-tx510 face scanner module is unstowed
	- And the test user's face has NOT been enrolled 
    - And the test user has been created in json user database 
    - And the test user's "FACEID" field is NULL
    

### Test Procedure
    1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Face Scan"
	4 Then the system goes to the Face Scan screen
    5 When the user selects "Begin Scan"
    6 And the user positions face on the hlk-tx510 display
    7 Then system sends Identification Command (0x12) to hlk-tx510
	8 And hlk-tx510 Identification Command returns result code 0x00 (success)
	9 And hlk-tx510 Identification Command returns "user_id"
   10 Then the system searches but does not find any user in the json user database that has a "FaceID" that matches the returned hlk-tx510 "user_id"
   11 Then the system displays "No user match - Enroll user"
   12 Then system stays on Face Scan screen
   13 When the user selects "Back" button
   14 Then the system returns to the biometric selection screen

### Test Results
    - Not tested
    - The cleanest unblock for Scenario 2 would be adding a "Clear Face ID" button to the user management screen — it sets the user's FACEID to -1
  in the JSON without needing to physically edit the SD card. That way you can:

   1. Clear Alice's FACEID
   2. Scan your face → it matches in the HLK-TX510 DB but finds no linked user → triggers the "No user match - Enroll user" message
   3. Re-enroll to restore it

   

## Scenario: 3 Face scan fails when face has been positioned on the hlk-tx510 display

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
	- And hlk-tx510 face scanner module is unstowed
	- And the test user's face has NOT been enrolled 
    - And face "user_id" is in the hlk-tx510 database
	- And the test user has been created in json user database 
    - And the test user's "FACEID" field is not NULL

### Test Procedure
	1 When the user selects "Begin authentication"
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
	

## Scenario: 4  Face scan fails because face has not been positioned on face scanner

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
	- And hlk-tx510 face scanner module is unstowed
	- And the test user's face has already been enrolled 
    - And face "user_id" is in the hlk-tx510 database
	- And the test user has been created in json user database 
    - And the test user's "FACEID" field is not NULL

### Test Procedure   
    1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Face Scan"
	4 Then the system goes to the Face Scan screen
    5 When the user selects "Begin Scan"
    6 And the user does not position face on hlk-tx510 display
    7 Then system sends Identification Command (0x12) to hlk-tx510
	8 Then hlk-tx510 Identification Command returns error code 0x01
	9 Then the system displays message "Face not detected - position face on scanner"
   10 Then system stays on Face Scan screen
   11 When the user selects "Back" button
   12 Then the system returns to the biometric selection screen

### Test Results
    - All steps pass


## Scenario: 5  User selects "Face Scan" on Biometric screen but hlk-tx510 face recognition scanner is stowed

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
	- And hlk-tx510 face scanner module is unstowed
	- And the test user's face has already been enrolled 
    - And face "user_id" is in the hlk-tx510 database
	- And the test user has been created in json user database 
    - And the test user's "FACEID" field is not NULL

### Test Procedure
	1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Face Scan"
    4 Then system goes to Face Scan screen
    5 Then system sends test command to hlk-tx510
    6 When system does not receive ACK from hlk-tx510
    7 Then system assumes hlk-tx510 is stowed (power off)
    8 Then system disables "Begin Scan" button
	9 Then the system displays message "Raise Face Scanner first"
   10 Then the system sends a test command to hlk-tx510 every 2 seconds
   11 When the system finally receives a valid ACK from hlk-tx510
   12 Then the system enables "Begin Scan" button
   13 Then the system displays "Face Scanner Raised"
   14 When the user selects "Begin Scan"
   15 Then the system begins Face Scan process

   ### Test Results
    - Step 9 failed
    - Expected: system displays message "Raise Face Scanner first"
    - Actual: System displays message "Scan error:ESP_ERR_IVALID_SIZE
    - Actual: System not yet configured to perform steps 5 through 13
    - Issue: issue-0002 identified. 5/01/2026
    - Issue: issue-0002 resolved. Implemented HLK-TX510 code to detect "Stowed" condition
   
