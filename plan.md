# Secure Lockbox ESP32-S3 — Session Plan

## Project
ESP32-S3 biometric security lockbox with LVGL GUI (Waveshare 800×480 RGB touch).
Authentication: fingerprint (R503), face (HLK-TX510), PIN. RTC: DS3231 via I2C.

## Current Task: RTC Date-Time Picker (Set Clock Screen)

### Completed
- [x] Full-screen overlay keypad for date-time entry
- [x] Field auto-advance (Day → Month → Year → Hour → Minute)
- [x] AM/PM toggle with 24h→12h conversion
- [x] Backspace with navigation-aware cursor (wraps across fields)
- [x] Live field validation (red flash when out of range)
- [x] Correct indicator flashes (date fields → date indicator, time fields → time indicator)
- [ ] Clear button resets all fields (NOT WORKING)
- [x] "Set Clock" button positioned left of Finger Unenroll in User Management screen
- [x] RTC read on screen open (populates fields with current RTC date-time)
- [x] Button sizing (140px wide, font 18) to fit all 3 buttons
- [x] No scrolling (scrollable flag removed from all containers)

### Remaining
- [ ] RTC shows default date (01/01/2000) if never set — expected behavior for unset DS3231

### Files Modified
- `main/ui/screens/ui_screen_rtc_date.c` — RTC date-time picker overlay
- `main/ui/screens/ui_screen_rtc_date.h` — public interface
- `main/user_mgmt_ui.c` — Set Clock button placement in User Management
- `main/devices/ds3231_rtc.c` — bus recovery fix
- `main/services/ds3231_service.c` — Kconfig/CMake/integration

### Tomorrow's Plan
1. Debug Clear button failure
2. Fix backspace cursor to properly jump to day field
3. Verify RTC shows correct date when RTC is set
