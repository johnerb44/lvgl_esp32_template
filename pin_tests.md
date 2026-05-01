# PIN Authentication Flow Testing

## Feature: Secure lockbox PIN authentication

### Background:
    - Given the secure lockbox is powered on
	- And there exists a user in the user database with Name = "Alice"
	- And the user named "Alice" has a PIN of "4321"
	- And the user has successfully authenticated using face or fingerprint
    - And the system is on the PIN entry screen

### Scenario: 1 User enters valid PIN
    - Then PIN Entry screen dislays message "Welcome, Alice" 
	- Then PIN Entry screen displays numeric keypad
	- Then "View PIN" button is not selected
	- Then "    " is displayed in PIN field
	- When the user enters PIN "4321" on keypad
	- Then "----" is displayed in the PIN field
	- When user presses "Backspace" button one time
	- Then "--- " is displayed in the PIN field
	- When user enters "1" on the numeric keypad
	- Then PIN Entry screen displays "----" in the PIN field
	- When user presses "Clear" on the numeric keypad
	- Then "    " is displayed in the PIN field
	- When the user selects the "View PIN" button
	- Then "    " is displayed in the PIN field
	- When the user enters PIN "4321" on keypad
	- Then "4321" is displayed in the PIN field
	- When the user selects the "Submit" button
	- Then the system attempts to match entered PIN with user's json user database PIN field
	- And PINs match
    - Then the system displays "PIN Accepted" for 3 seconds
    - Then the system proceeds to the Home screen


### Scenario: 2 User enters invalid PIN
    - Then PIN Entry screen dislays message "Welcome, Alice" 
	- Then PIN Entry screen displays numeric keypad
	- Then "View PIN" button is not selected
	- Then "    " is displayed in PIN field
	- When the user enters PIN "5554" on keypad
	- Then "----" is displayed in the PIN field
	- When user presses "Backspace" button one time
	- Then "--- " is displayed in the PIN field
	- When user enters "4" on the numeric keypad
	- Then PIN Entry screen displays "----" in the PIN field
	- When user presses "Clear" on the numeric keypad
	- Then "    " is displayed in the PIN field
	- When the user selects the "View PIN" button
	- Then "    " is displayed in the PIN field
	- When the user enters PIN "5554" on keypad
	- Then "5554" is displayed in the PIN field
	- When the user presses the "Submit" button
	- Then the system attempts to match entered PIN with user's json user database PIN field
	- And PINs do not match
    - Then the system displays "Invalid PIN" for 5 seconds
	- When the user enters a valid PIN in less than 10 tries
	- Then the system attempts to match entered PIN with user's json user database PIN field
	- And PINs match
    - Then the system displays "PIN Accepted" for 3 seconds
    - Then the system proceeds to the Home screen
	- When the user enters an invalid PIN 10 times
    - Then the system stays on PIN Entry screen	
	- When user enters an invalid PIN ten times
	- Then the system does not allow the user to try authentication again for 60 seconds
	- Then system proceeds to the Main screen
	
	
	
