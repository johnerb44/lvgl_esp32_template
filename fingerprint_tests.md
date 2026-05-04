# Secure lockbox fingerprint biometric authentication


## Scenario: 1 Fingerprint Scan - Finger Found in R503 database and Finger Match found in user database

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
    - And the test user has a valid user account
    - And test user's "FINGERID" field is not null (ie test user has enrolled their fingerprint)
    - And system is configured to use the R503 automatic fingerprint verification command (0x32)

### Test Procedure
    1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Fingerprint Scan"
	4 Then the system goes to the Fingerprint Scan screen
    5 When the user selects "Begin Scan"
    6 Then the system sends Automatic Fingerprint Verification Command (AutoIdentify 0x32) to R503
    7 Then the R503 LED "breaths White light"
    8 Then the user places a previously scanned finger on the fingerprint scanner
    9 When the R503 completes sucessful fingerprint image collection
   10 Then the R503 returns Step 1 packet (image collection) confirmation success code 0x00
   11 Then the R503 LED lights up Yellow
   12 Then the R503 Generates the fingerprint feature
   13 Then the R503 returns Step 2 packet (feature generation) confirmation success code 0x00
   14 Then the R503 searches and compares the image with fingerprint templates in the R503 database
   15 When the R503 finds a matching fingerprint template in the R503 fingerprint database
   16 Then the R503 returns Step 3 packet (search) confirmation success code 0x00
   17 And the R503 returned Step 3 packet contains ModelID
   18 Then the R503 LED lights up Green
   19 Then the system matches user's "FINGERID" field to R503 returned "ModelID"
   20 When the match is sucessful
   21 Then the system displays message "Fingerprint found - User match found"
   22 Then system stays on Fingerprint Scan screen for 5 seconds
   23 Then the system goes to the PIN Entry screen
	
### Test Results
    - Step 6 fails
    - Expected: System should send Automatic Fingerprint Verification Command (AutoIdentify 0x32) to R503 
    - Actual: System is not configured to use Command AutoIdentify (0x32) for fingerprint identification
    - Issue: issue-0004 identified 05/03/2026



## Scenario: 2 Fingerprint is found in the R503 database but does not match any registered user

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
    - And the test user has a valid user account
    - And test user's "FINGERID" field is null (ie test user has NOT enrolled their fingerprint)
    - And system is configured to use the R503 automatic fingerprint verification command (AutoIdentify 0x32)
  
### Test Procedure
    1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Fingerprint Scan"
	4 Then the system goes to the Fingerprint Scan screen
    5 When the user selects "Begin Scan"
    6 Then the system sends Automatic Fingerprint Verification Command (AutoIdentify 0x32) to R503
    7 Then the R503 LED "breaths White light"
    8 And the user places a previously scanned finger on the fingerprint scanner
    9 When the R503 completes sucessful fingerprint image collection
   10 Then the R503 returns Step 1 packet (image collection) confirmation success code 0x00
   11 Then the R503 LED lights up Yellow
   12 Then the R503 Generates the fingerprint feature
   13 Then the R503 returns Step 2 packet (feature generation) confirmation success code 0x00
   14 Then the R503 searches and compares the image with fingerprint templates in the R503 database
   15 When the R503 finds a matching fingerprint template in the R503 fingerprint database
   16 Then the R503 returns Step 3 packet (search) confirmation success code 0x00
   17 And the R503 returned Step 3 packet contains ModelID
   18 Then the R503 LED lights up Green
   19 Then the system searches for a match of user's "FINGERID" field to R503 returned "ModelID"
   20 When the system fails to find a match of user's "FINGERID" field to R503 returned "ModelID" 
   21 Then the system sends the R503 an Aural command R503 to blink the LED Purple three times 
   22 Then the system displays message "Fingerprint found - User match not found"
   23 Suggest user re-enroll fingerprint here????
   24 Then system stays on Fingerprint Scan screen
   25 When the user selects "Back" button
   26 Then the system returns to the biometric selection screen
   
   ### Test Results
    - Step 6 fails
    - Expected: System should send Automatic Fingerprint Verification Command (AutoIdentify 0x32) to R503 
    - Actual: System is not configured to use Command AutoIdentify (0x32) for fingerprint identification
    - Issue: issue-0005 identified 05/03/2026



## Scenario: 3 Fingerprint is not found in the R503 database and finger has been placed on Fingerprint scanner

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
    - And the test user has a valid user account
    - And test user's "FINGERID" field is null (ie test user has NOT enrolled their fingerprint)
    - And system is configured to use the R503 automatic fingerprint verification command (0x32)
  
  ### Test Procedure
    1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Fingerprint Scan"
	4 Then the system goes to the Fingerprint Scan screen
    5 When the user selects "Begin Scan"
    6 Then the system sends Automatic Fingerprint Verification Command (AutoIdentify 0x32) to R503
    7 Then the R503 LED "breaths White light"
    8 And the user places a finger on the fingerprint scanner
    9 When the R503 completes sucessful fingerprint image collection
   10 Then the R503 returns Step 1 packet (image collection) confirmation success code 0x00
   11 Then the R503 LED lights up Yellow
   12 Then the R503 Generates the fingerprint feature
   13 Then the R503 returns Step 2 packet (feature generation) confirmation success code 0x00
   14 Then the R503 searches and compares the image with fingerprint templates in the R503 database
   15 When the R503 does NOT find a matching fingerprint template in the R503 fingerprint database
   16 Then the R503 returns Step 3 packet (search) confirmation success code 0x09
   17 Then the R503 LED blinks Red 3 times
   18 Then the system displays message "Fingerprint not found - See Admin - enroll finger" (Or try another finger????)
   19 Then system stays on Fingerprint Scan screen
   20 When the user selects "Back" button
   21 Then the system returns to the biometric selection screen

### Test Results
    - Not tested
	


## Scenario: 4 Fingerprint is not found in the R503 database and finger has not been placed on R503  Fingerprint scanner (error code 0x02: no finger on the sensor)

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
    - And the test user has a valid user account
    - And test user's "FINGERID" field is null (ie test user has NOT enrolled their fingerprint)
    - And system is configured to use the R503 automatic fingerprint verification command (0x32)

### Test Procedure
    1 When the user selects "Begin authentication"
	2 Then the system goes to Biometric screen
    3 When the user selects "Fingerprint Scan"
	4 Then the system goes to the Fingerprint Scan screen
    5 When the user selects "Begin Scan"
    6 Then the system sends Automatic Fingerprint Verification Command (AutoIdentify 0x32) to R503
    7 Then the R503 LED "breaths White light"
    8 When the user does not place a finger on the fingerprint scanner
    9 Then the R503 returns Step 1 packet (image collection) confirmation code 0x26 (Times Out)
   10 Then the fingerprint scanner LED blinks Red 3 times
   11 Then the system displays message "Finger not detected - place finger on scanner - try again"
   12 Then system stays on Fingerprint Scan screen
   13 When the user selects "Back" button
   14 Then the system returns to the biometric selection screen

### Test Results
    - Not tested

 
