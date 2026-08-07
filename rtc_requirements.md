# Secure Lockbox System Reat Time Clock (RTS) Implementation
This plan provides a comprehensive approach to implementing RTC functionality that integrates with the existing architecture while preserving all critical system features. The implementation will be modular and extensible for future enhancements.

## Real Time Clock Requirements
Based on the project architecture, implement Real Time Clock functionality that:

- Enhances the security of the Secure Lockbox System
- Provides a record of the last date each user has logged on
- Integrates with the User Management function
- Integrates with the user json database
- Meets the goal of keeping the system useable by 8 - 12 year olds

## Expected Benefits
- Clearly show a system administrator who is accessing the system and when
- Requires the admin to set the RTC clock as seldom as possible
- Is invisible to a non-admin user
- Maintains security and functionality


## Real Time Clock Constraints
- Setting the date-time is ONLY initiated from the User Management screen
- Date-time changes cannot be initiated by normal users
- Maintaining date-time is off-loaded from the main processor
- The date-time should be maintained when the system is powered down (battery backup)
- The RTC must maintain time information even though the json user LAST_LOGON field will contain only day-month-year information
  

## Date-time setting Criteria
- The User Management screen has a Set Date-Time button
- The Set date-time is only accessed by an Admin
- Useability - Setting date-time is simple and intuitive
- Setting date-time is via a pop-up screen from the User Management screen
- Use the date-time entry screen the Get PIN screen as a template
- The Set Date-time screen uses a virtual numeric keypad
- The set Date-time function will set day-month-year and hour-minute-second
- Seconds will always be passed to the RTC as zero
- Setting the date-time will assume a 12 hour clock and provide an AM/PM check box to the Admin user
- The set date-time screen will validate user input: day (1-31) month (1-12) year (2000-2099) hour (1-12) minute (0-59) 

## Updating the json User Database
**Objective**: Implement mechanisms to update the LAST_LOGON field
- The LAST_LOGON field of json user database record the current user will be updated immediately after the user has entered a valid PIN
- LAST_LOGON is updated by the Get PIN screen, before transitioning to the next screen
- LAST_LOGON field is a string field in the format of dd-mm-yyyy
- LAST_LOGON field will not contain hour-minute-second or AM-PM information


## Integration with Existing System
**Objective**: Seamlessly integrate Real Time Clock (RTC) with current architecture

1. **UI Integration**
   - Set date-time screen called from USER Management screen
   - Display LAST-LOGON field as part of User information in the User Management screen user dropdown list

2. **Service Integration**
   - Modify User Management screen to work with Real Time clock (set date-time and display user record information)
   - Update authentication service for RTC LAST_LOGON update scenarios
   - Integrate with existing battery monitoring


## Real Time Clock Hardware
- System Interface board (separate from the Waveshare ESP32-S3 LCD Display-4.3 module) includes an HiLetgo DS3231/DS3231SN Real Time Clock module
- DS3231 contains a replaceable CR1220 3v Lithium battery backup
- DS3231 is an I2C accessible chip
- DS3231 is at I2C address 0x68


## Testing and Verification
- Verify that DS3231 holds date-time when system is powered down
- Verify that DS3231 is available at I2C address 0x68
- Verify that user LAST_LOGON field is updated with current date after valid PIN verification
- Verify that Set date-time screen works 
- Verify that the set date-time screen validates user date-time input
- Verify that the user record shown in the User dropdown list on the User Management screen displays correct last logon date



  

