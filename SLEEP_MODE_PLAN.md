# Sleep Mode Implementation Plan

## Project Overview
This is an ESP-IDF project for a Secure Lockbox system using ESP32-S3 with biometric authentication, servo-controlled locking, and LVGL GUI. The system needs sleep mode functionality to conserve power when idle.

## Current System State
The system is already quite complete with:
- Biometric authentication (fingerprint and face recognition)
- Servo-controlled locking mechanism (PCA9685)
- LCD display with LVGL GUI
- Battery monitoring (both ADC and INA219 approaches)
- SD card user storage
- I2C-to-UART bridge for biometric modules

## Sleep Mode Requirements
Based on the project architecture, I need to implement sleep mode functionality that:

1. **Reduces power consumption** when the system is idle
2. **Preserves critical functionality** (user authentication, lock status)
3. **Supports wake-up triggers** (user interaction, biometric input, timer)
4. **Maintains system state** (user sessions, lock status)
5. **Integrates with existing components** (LCD, sensors, servo)

## Implementation Approach

### Phase 1: Sleep Mode Core Architecture
**Objective**: Create a sleep mode framework that can be extended for different sleep levels

1. **Create Sleep Service** (`services/sleep_service.h/.c`)
   - Initialize sleep functionality
   - Manage sleep state transitions
   - Provide sleep/wake APIs
   - Handle sleep timer configuration

2. **Sleep State Management**
   - Define sleep levels (deep sleep, light sleep, idle)
   - Implement state machine for sleep transitions
   - Handle wake-up conditions

### Phase 2: Hardware Power Management
**Objective**: Implement power control for key components

1. **LCD Power Control**
   - Implement backlight control via CH422G driver
   - Disable LCD when sleeping
   - Restore LCD on wake

2. **Sensor Power Management**
   - Control power to biometric sensors (fingerprint, face recognition)
   - Implement sensor wake-up sequences
   - Preserve sensor state during sleep

3. **Servo Power Management**
   - Control servo power to reduce standby current
   - Maintain lock position during sleep

### Phase 3: Wake-up Triggers
**Objective**: Implement mechanisms to wake the system from sleep

1. **Touch Wake-up**
   - Configure touch controller for wake-up capability
   - Implement touch event handling during sleep

2. **Timer Wake-up**
   - Implement periodic wake-up for status checks
   - Configure RTC timer for sleep intervals

3. **Sensor Wake-up**
   - Wake-up from biometric input
   - Handle authentication during wake

### Phase 4: Integration with Existing System
**Objective**: Seamlessly integrate sleep mode with current architecture

1. **UI Integration**
   - Update home screen to show sleep status
   - Add sleep mode controls
   - Display sleep timer countdown

2. **Service Integration**
   - Modify lock service to work with sleep mode
   - Update authentication service for sleep/wake scenarios
   - Integrate with existing battery monitoring

3. **Application Integration**
   - Update `lockbox_app.c` to initialize sleep service
   - Implement automatic sleep after inactivity
   - Handle session state during sleep

## Detailed Implementation Steps

### Step 1: Create Sleep Service Interface
Create `main/services/sleep_service.h` and `main/services/sleep_service.c` with:
- Sleep initialization
- Sleep state management
- Sleep timer functionality
- Wake-up handling

### Step 2: Implement Sleep State Machine
Implement sleep states:
- `SLEEP_STATE_ACTIVE` - Normal operation
- `SLEEP_STATE_IDLE` - System idle, LCD off
- `SLEEP_STATE_DEEP_SLEEP` - Full system sleep

### Step 3: Power Management Integration
1. **LCD Power Control**
   - Add backlight control to `ch422g_driver.c`
   - Implement LCD off during sleep
   - Restore LCD on wake

2. **Sensor Power Control**
   - Implement sensor power management in `sc16is752_transport.c`
   - Add sensor wake-up functionality

3. **Servo Power Control**
   - Add servo power management to `pca9685_device.c`
   - Preserve lock position during sleep

### Step 4: Wake-up Mechanisms
1. **Touch Wake-up**
   - Configure GT911 touch controller for wake-up
   - Implement touch event handling

2. **Timer Wake-up**
   - Implement RTC timer for periodic wake-ups
   - Add sleep timer configuration

3. **Sensor Wake-up**
   - Handle biometric input wake-up
   - Implement authentication on wake

### Step 5: UI Integration
1. **Home Screen Updates**
   - Add sleep status indicator
   - Add sleep timer display
   - Implement sleep mode controls

2. **Sleep Mode Screen**
   - Create dedicated sleep mode screen
   - Show battery level during sleep
   - Display wake-up status

### Step 6: System Integration
1. **Automatic Sleep**
   - Implement automatic sleep after inactivity
   - Configure sleep timeout period
   - Handle session state preservation

2. **Session Management**
   - Preserve user sessions during sleep
   - Handle lock status preservation
   - Implement secure wake-up

## Configuration Options
Add to `main/Kconfig.projbuild`:
```
config LOCKBOX_FEATURE_SLEEP_MODE
    bool "Enable sleep mode"
    default n
    depends on LOCKBOX_INTEGRATION_ENABLE
    help
        Enables sleep mode functionality to reduce power consumption
```

## Testing and Verification
1. **Power Consumption Testing**
   - Measure power usage in different sleep states
   - Verify sleep timer accuracy

2. **Wake-up Testing**
   - Test touch wake-up functionality
   - Test timer wake-up functionality
   - Test sensor wake-up functionality

3. **Integration Testing**
   - Verify system state preservation during sleep
   - Test UI behavior during sleep/wake cycles
   - Validate authentication flow after wake-up

## Expected Benefits
- Reduced power consumption during idle periods
- Extended battery life for portable system
- Improved system efficiency
- Maintained security and functionality

This plan provides a comprehensive approach to implementing sleep mode functionality that integrates with the existing architecture while preserving all critical system features. The implementation will be modular and extensible for future enhancements.