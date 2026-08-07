# Secure Lockbox System Sleep Mode
This plan provides a comprehensive approach to implementing sleep mode functionality that integrates with the existing architecture while preserving all critical system features. The implementation will be modular and extensible for future enhancements.

## Sleep Mode Requirements
Based on the project architecture, implement sleep mode functionality that:

1. **Reduces power consumption** when the system is idle
2. **Preserves critical functionality** (user authentication, lock status)
3. **Supports wake-up triggers** (user interaction, RTC timer)
4. **Maintains system state** (user sessions, lock status)
5. **Integrates with existing components** (LCD, sensors, servo, etc.)

## Expected Benefits
- Reduced power consumption during idle periods
- Extended battery life for portable system
- Improved system efficiency
- Maintained security and functionality


## Sleep Mode Constraints
- Sleep Mode is ONLY initiated when entering the main screen
- Sleep mode cannot be initiated from/by any screen except the main screen
- Sleep mode coundown and wake-up timers stop and are cleared when the system leaves the main screen

## Sleep Mode Initiation Criteria
- All users are logged off system
- HLK-TX510 Face Recognition Module is Stowed
- LID is closed
- Lock Status is Locked
- UI is on Main (Start) screen
- Sleep Mode process is initiated when entering the main screen including at restart/reboot time


## Wake-up Triggers
**Objective**: Implement mechanisms to wake the system from sleep

1. **Timer Wake-up**
   - Implement periodic wake-up to check the status of the R503 touch/Interrupt Wakeup signal
   - Configure RTC timer for sleep intervals

2. **Sensor Wake-up**
   - Wake-up from biometric input (R503 Wake-up/Touch/Interrupt signal)


## Integration with Existing System
**Objective**: Seamlessly integrate sleep mode with current architecture

1. **UI Integration**
   - Display sleep timer countdown

2. **Service Integration**
   - Modify lock service to work with sleep mode
   - Update authentication service for sleep/wake scenarios
   - Integrate with existing battery monitoring

3. **Application Integration**
   - Update `lockbox_app.c` to initialize sleep service
   - Implement automatic sleep after inactivity
   - Handle session state during sleep


## Step 1: Create Sleep Service Interface
Create `main/services/sleep_service.h` and `main/services/sleep_service.c` with:
- Sleep initialization
- Sleep state management
- Sleep timer functionality
- Wake-up handling


## Step 2: Implement Sleep State Machine
Implement sleep states:
- `SLEEP_STATE_ACTIVE` - Normal operation
- `SLEEP_STATE_IDLE` - System idle, LCD off
- `SLEEP_STATE_DEEP_SLEEP` - Full system sleep


## Step 3: Power Management Integration
1. **LCD Power Control**
   - Add backlight control to `ch422g_driver.c`
   - Implement LCD off during sleep
   - Restore LCD on wake

2. **Sensor Power Control**
   - Implement sensor power management in `sc16is752_transport.c`
   - Add sensor wake-up functionality

3. **Servo Power Control**
   - Add servo power management to `pca9685_device.c`
   - Set PWM to 0
   - Preserve lock position during sleep


## Step 5: UI Integration
1. **Main Screen Updates**
   - At screen loading initiate 5 minute countdown timer
   - Display "System shutdown initiated" 
   - When 5 minute countdown timer is at 1 minute replace "System Shutdown initiated" with 1 minute countdown display "System Shutdown in 60s...59s...58s...3s...2s...1s" 
   - Implement sleep mode controls
   - If the user selects either "Begin Authentication" button or "First Time Setup" button at anytime during the 5 minute countdown the system will move to another screen and stop the sleep/shut down process and clear all sleep timers


## Step 6: System Integration
1. **Automatic Sleep**
   - Implement automatic sleep after inactivity (ONLY while on the main screen)
   - Configure sleep timeout period
   - Handle session state preservation

2. **Session Management**
   - Preserve user sessions during sleep
   - Handle lock status preservation
   - Implement secure wake-up
 
 3. **R503 Fingerprint Scanner
   - Has "Wake-up/Touch/Interrupt" signal to trigger system wake-up
   - R503 "Wake-up/Touch/Interrupt" signal is connected to SC16IS752 chip, GP1
   - R503 "Wake-up/Touch/Interrupt" signal is HIGH when R503 is NOT touched
   - R503 "Wake-up/Touch/Interrupt" signal is LOW when R503 IS touched


## Testing and Verification
1. **Power Consumption Testing**
   - Measure power usage in different sleep states
   - Verify sleep timer accuracy

2. **Wake-up Testing**
   - Test countdown timer functionality (countdown from 5 minutes)
   - Test timer wake-up functionality (wake-up every 20 seconds)
   - Test sensor wake-up functionality (check status of the R503 Touch/Interrupt wake-up signal once every 1 seconds for 10 seconds)

3. **Integration Testing**
   - Verify system state preservation during sleep
   - Test UI behavior during sleep/wake cycles
   - Validate authentication flow after wake-up


## Enter Sleep Mode
- When System is on the main screen
- When All initiation criteria are met
- When 5 minute countdown timer reaches zero
- Then the LCD display/backlight is turned off
- Then the SC16IS752 and PCA9685 chips are put in sleep mode (if applicable)
- Then the ESP32S3 processor is placed in sleep mode (Deep sleep if possible)
- ESP32S3 is set to "wake-up" every 20 seconds using the internal RTC


## Exit Sleep Mode
- ESP32S3 is set to "wake-up" every 20 seconds using the internal RTC
- When processor wakes-up (minimally) it sends command to SC16IS752 to wake this chip up
- For 10 seconds, at 1 second intervals, the processor queries the SC16IS752 GP1 pin for a LOW level signal (this comes from the R503)
- If the GP1 pin is detected as LOW at anytime during the 10 second wake-up interval then the system executes code to completely come out of sleep mode (wake-up)
- If the GP1 pin stays HIGH during that 10 second interval then the system goes back into Deep sleep for 20 seconds again and the process continues
  

