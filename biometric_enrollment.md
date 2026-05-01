# Biometric Enrollment Testing

## Feature: Secure lockbox user fingerprint biometric enrollment

### Background:
    - Given the secure lockbox is powered on
    - And the system is on the Main screen
    - And the test user has a valid user account
    - And test user's fingerprint has not been enrolled
    - And the test user has been assigned a valid PIN in the user database
    - And the test user has been notified, by the Administrator, what their new PIN is
    - And system is configured to use the R503 automatic fingerprint enrollment (AutoEnroll) command (0x31)


### Scenario: Fingerprint Scan - New user, first time fingerprint enrollment 
    - Given the user is intentionally enrolling fingerprint for the first time
    - When the user selects "Begin authentication"
	- Then the system goes to Biometric screen
    - When the user selects "Fingerprint Scan"
	- Then the system goes to the Fingerprint Scan screen
    - When the user selects "Enroll Fingerprint"
    1 LED flashes Blue
    1 Place finger on fingerprint scanner
    1 Led flashes Yellow (Scan image complete)
    1 LED Flashes Green (Scan success)
    1 Led flashes White. 
    1 Remove finger
    
    2 LED flashes Blue
    2 Place Finger on Scanner
    2 LED flashes yellow
    2 LED Flashes Green
    2 Led flashes white
    2 Remove Finger
      
    Six times

    

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


### Scenario: Fingerprint Scan - Finger Found in R503 database and no Finger Match found in user database
    - When the user selects "Begin authentication"
	- Then the system goes to Biometric screen
    - When the user selects "Fingerprint Scan"
	- Then the system goes to the Fingerprint Scan screen
    - When the user selects "Begin Scan"
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