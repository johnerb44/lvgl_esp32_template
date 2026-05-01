# Authentication Flow Testing

## Fingerprint Scan - Finger Found in R503 database- No Match to User in user database

- tested
- action: boot
- goto: ui_screen_main
- action: press "Begin authentication" button
- goto: ui_screen_biometric
- action: press "Fingerprint Scan" button
- goto: ui_screen_fpscan
- action: press "Begin Scan" button
- action: place previously scannned finger on fingerprint scanner
- observe: fingerprint scanner LED blinks Blue 3 times
- observe: fingerprint scanner LED blinks Green 3 times
- observe: message reads "No match (ok)"
- action: press "Back" button
- goto: ui_screen_biometric
- stop
- expected: observe: message reads "Fingerprint found: does not match any user"
- expected: goto: ui_screen_biometric


## Fingerprint Scan - Finger Found in R503 database -  Finger Match found in user database - PIN Match

- not tested




## Fingerprint Scan - Finger Not Found in R503 database

- tested
- action: boot
- goto: ui_screen_main
- action: press "Begin authentication" button
- goto: ui_screen_biometric
- action: press "Fingerprint Scan" button
- goto: ui_screen_fpscan
- action: press "Begin Scan" button
- action: place not previously scannned finger on fingerprint scanner
- observe: fingerprint scanner LED blinks Blue 3 times
- observe: fingerprint scanner LED blinks Red 3 times
- observe: message reads "No match (no match)"
- action: press "Back" button
- goto: ui_screen_biometric
- stop
- expected: observe: message reads "Fingerprint not found" or "Fingerprint not found: try another finger"
- expected: goto: ui_screen_biometric


# Face Scan - Face found in hlk-tx510 database  - Face ID found in user database - PIN Match

- tested
- action: boot
- goto: ui_screen_main
- action: press "Begin authentication" button
- goto: ui_screen_biometric
- action: press "Face Scan" button
- goto: ui_screen_facescan
- action: position face on hlk-tx510 screen
- action: press "Begin Scan" button
- observe: changes screen immediately - no time to view any message on ui_screen_facescan
- goto: ui_screen_get_pin 
- action: enter PIN (4321) from onscreen keypad 
- action: press "Submit" button
- observer: momentary (about 2 seconds) message in upper left corner "PIN Accepted Unlocked"
- goto: ui_screen_home
- observe: message "Welcome, Alice" centered directly under "Secure Lock Box" screen title
- observe: status message in upper right corner "[LOCKED]"
- action: press "1 Unlock Secure Box" button
- observe: momentary (about 2 seconds) message in upper left corner "Box Unlocked"
- observe: status message in upper right corner changes to "[UNLOCKED]"
- stop
- expected: observe: pause 2-3 seconds on successful face match before going to ui_screen_home so user can see face match message
- expected: goto: ui_screen_get_pin on successful face match
- expected: goto: ui_screen_home on successful PIN match


## Face Scan - Face Found in hlk-tx510 database -  Face ID found in user database - No PIN Match

- not tested


## Face Scan - Face Not Found in hlk-tx510 database

- not tested


