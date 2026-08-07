# I2C Communication Restoration Plan

**Date:** August 7, 2026  
**Issue:** All I2C communication to external devices has stopped working  
**Root Cause:** System running in MOCK_DEVICES mode with all hardware features disabled

---

## ANALYSIS SUMMARY

### What Changed?
The application's configuration has been set to **MOCK_DEVICES mode**, which bypasses all real I2C hardware communication. This was detected by analyzing the ESP-IDF build log warnings showing static helper functions marked as "defined but not used."

### Why Are Functions Unused?
All unused functions are conditional I2C operations that only execute when:
- `CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES=n` (currently set to `y`)
- Hardware-specific feature flags are enabled

#### Unused Functions by Module:

**SC16IS752 Transport** (`main/comm/sc16is752_transport.c`):
- `sc16is752_transport_read_timed()` - blocked by mock mode
- `configure_sc16_uart()` - blocked by mock mode  
- `wait_for_tx_ready()` - blocked by mock mode
- `wait_for_tx_complete()` - blocked by mock mode
- `ensure_i2c_ready()` - blocked by mock mode

**PCA9685 Servo Driver** (`main/devices/pca9685_device.c`):
- `pca9685_set_frequency()` - blocked by mock mode or `CONFIG_LOCKBOX_FEATURE_PCA9685=n`
- `servo_sweep_task()` - blocked by mock mode or feature disabled
- `ensure_i2c_ready()` - blocked by mock mode or feature disabled

**Root Cause Sequence:**
```
idf.py build
  ↓
main/comm/sc16is752_transport_init()
  ↓
if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES == y:
  return ESP_OK immediately (mock mode activated)
  ↓
  configure_sc16_uart() NEVER CALLED
  wait_for_tx_ready() NEVER CALLED
  wait_for_tx_complete() NEVER CALLED
  ↓
compiler detects unused static functions → warnings
```

### Current Configuration (sdkconfig.defaults)
```
CONFIG_LOCKBOX_INTEGRATION_ENABLE=y              ✓ Enabled
CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES=y   ✗ BLOCKING I2C (MUST DISABLE)
CONFIG_LOCKBOX_FEATURE_SC16IS752=n              ✗ I2C→UART bridge disabled
CONFIG_LOCKBOX_FEATURE_R503=n                   ✗ Fingerprint disabled
CONFIG_LOCKBOX_FEATURE_HLK_TX510=n              ✗ Face recognition disabled
CONFIG_LOCKBOX_FEATURE_PCA9685=n                ✗ Servo driver disabled
CONFIG_LOCKBOX_FEATURE_DS3231=n                 ✗ RTC disabled
CONFIG_LOCKBOX_FEATURE_INA219=n                 ✗ Battery monitor disabled
CONFIG_LOCKBOX_FEATURE_STATUS_INPUTS=y          ✓ Status inputs enabled
CONFIG_LOCKBOX_FEATURE_SLEEP_MODE=n             ✗ Sleep mode disabled
```

---

## RESTORATION PROCEDURE

### Step 1: Disable Mock Mode

**File:** `sdkconfig.defaults`

**Change:**
```diff
  CONFIG_LOCKBOX_INTEGRATION_ENABLE=y
- CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES=y
+ CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES=n
```

**Effect:** Activates real I2C hardware communication paths throughout the codebase.

---

### Step 2: Enable SC16IS752 Transport (Required for Any Biometric)

**File:** `sdkconfig.defaults`

**Add/Change:**
```diff
+ CONFIG_LOCKBOX_FEATURE_SC16IS752=y
```

**Purpose:** 
- Enables the I2C→dual-UART bridge that connects fingerprint (R503) and face (HLK-TX510) sensors
- This is the **prerequisite for all biometric features**
- Opens I2C bus communication at address 0x4D

---

### Step 3: Enable Hardware Features (Select Based on Your System)

Choose which external devices you need to support:

#### **Option A: Fingerprint Only**
```
CONFIG_LOCKBOX_FEATURE_R503=y
```
- Unlocks fingerprint authentication
- SC16IS752 channel A @ 57600 baud

#### **Option B: Face Recognition Only**
```
CONFIG_LOCKBOX_FEATURE_HLK_TX510=y
```
- Unlocks face authentication
- SC16IS752 channel B @ 115200 baud

#### **Option C: Fingerprint + Face (Full Biometric)**
```
CONFIG_LOCKBOX_FEATURE_R503=y
CONFIG_LOCKBOX_FEATURE_HLK_TX510=y
```
- Both channels active simultaneously
- Channel A: fingerprint @ 57600
- Channel B: face @ 115200

#### **Option D: Servo/Lock Control**
```
CONFIG_LOCKBOX_FEATURE_PCA9685=y
```
- Enables servo driver for physical lock mechanism
- Communicates over I2C at address 0x40

#### **Option E: RTC (Date/Time Stamping)**
```
CONFIG_LOCKBOX_FEATURE_DS3231=y
```
- Enables DS3231 RTC for persistent date/time
- Used for "last login" timestamps
- Communicates over I2C at address 0x68

#### **Option F: Battery Monitoring**
```
CONFIG_LOCKBOX_FEATURE_INA219=y
```
- Enables power/battery status monitoring
- Displays voltage and charge percentage
- Communicates over I2C at address 0x41

#### **Option G: Sleep Mode**
```
CONFIG_LOCKBOX_FEATURE_SLEEP_MODE=y
```
- Requires: `CONFIG_LOCKBOX_FEATURE_STATUS_INPUTS=y` (already enabled)
- Enables low-power sleep when idle

---

### Step 4: Apply Configuration

After editing `sdkconfig.defaults`, rebuild:

```bash
cd /home/erbj/lvgl-projects/lvgl_esp32_template

# Clean rebuild to ensure config is applied
idf.py fullclean
idf.py build

# Flash to device
idf.py -p <PORT> flash monitor
```

**Expected Output in Serial Log:**
```
I (xxxxx) SC16IS752_TRANSPORT: SC16IS752 transport initialized (A=57600, B=115200)
I (xxxxx) R503: R503 device ready on SC16IS752 CH_A at 57600 baud
I (xxxxx) HLK_TX510: HLK-TX510 face module ready on SC16IS752 CH_B at 115200 baud
I (xxxxx) PCA9685_DEVICE: PCA9685 initialized at address 0x40
I (xxxxx) DS3231_RTC: DS3231 initialized OK (port=0, s_initialized=1)
I (xxxxx) INA219: INA219 initialized at I2C address 0x41
```

---

## VERIFICATION CHECKLIST

After applying changes, verify:

- [ ] **Build completes successfully** - No errors
- [ ] **Compiler warnings eliminated** - Previously unused functions now used
  - `configure_sc16_uart()` used
  - `wait_for_tx_ready()` used
  - `wait_for_tx_complete()` used
  - `pca9685_set_frequency()` used
  - `servo_sweep_task()` used
- [ ] **Serial log shows hardware init messages** (not mock mode)
- [ ] **SC16IS752 probe succeeds** (if enabled)
  ```
  I (xxxxx) SC16IS752_TRANSPORT: Probe: SC16IS752 verified at I2C addr 0x4D
  ```
- [ ] **Fingerprint sensor works** (if `CONFIG_LOCKBOX_FEATURE_R503=y`)
  - Try "Authenticate Fingerprint" → should detect finger placement
- [ ] **Face recognition works** (if `CONFIG_LOCKBOX_FEATURE_HLK_TX510=y`)
  - Try "Authenticate Face" → should detect face
- [ ] **Servo unlocks/locks** (if `CONFIG_LOCKBOX_FEATURE_PCA9685=y`)
  - Physical lock mechanism should respond to commands
- [ ] **RTC shows current date** (if `CONFIG_LOCKBOX_FEATURE_DS3231=y`)
  - Admin screen should display RTC date/time
- [ ] **Battery percentage shows** (if `CONFIG_LOCKBOX_FEATURE_INA219=y`)
  - Home screen should display battery voltage and percentage

---

## RECOMMENDED CONFIGURATIONS

### Configuration A: Full-Featured Secure Lockbox
```
CONFIG_LOCKBOX_INTEGRATION_ENABLE=y
CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES=n
CONFIG_LOCKBOX_FEATURE_SC16IS752=y
CONFIG_LOCKBOX_FEATURE_R503=y
CONFIG_LOCKBOX_FEATURE_HLK_TX510=y
CONFIG_LOCKBOX_FEATURE_PCA9685=y
CONFIG_LOCKBOX_FEATURE_DS3231=y
CONFIG_LOCKBOX_FEATURE_INA219=y
CONFIG_LOCKBOX_FEATURE_STATUS_INPUTS=y
CONFIG_LOCKBOX_FEATURE_SLEEP_MODE=y
```

### Configuration B: Fingerprint Only (Minimal)
```
CONFIG_LOCKBOX_INTEGRATION_ENABLE=y
CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES=n
CONFIG_LOCKBOX_FEATURE_SC16IS752=y
CONFIG_LOCKBOX_FEATURE_R503=y
CONFIG_LOCKBOX_FEATURE_HLK_TX510=n
CONFIG_LOCKBOX_FEATURE_PCA9685=n
CONFIG_LOCKBOX_FEATURE_DS3231=n
CONFIG_LOCKBOX_FEATURE_INA219=n
CONFIG_LOCKBOX_FEATURE_STATUS_INPUTS=y
CONFIG_LOCKBOX_FEATURE_SLEEP_MODE=n
```

### Configuration C: Development/Testing (Mock Mode)
```
CONFIG_LOCKBOX_INTEGRATION_ENABLE=y
CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES=y  ← Stays in mock mode
```
(Use when testing UI without hardware present)

---

## I2C DEVICE ADDRESS MAP

After full restoration, the I2C bus (I2C_NUM_0, GPIO8=SDA, GPIO9=SCL) will have:

| Device | Address | Feature Flag | Purpose |
|--------|---------|--------------|---------|
| SC16IS752 | 0x4D | CONFIG_LOCKBOX_FEATURE_SC16IS752 | Dual UART bridge (fingerprint + face) |
| PCA9685 | 0x40 | CONFIG_LOCKBOX_FEATURE_PCA9685 | Servo/lock driver |
| DS3231 | 0x68 | CONFIG_LOCKBOX_FEATURE_DS3231 | RTC / date-time |
| INA219 | 0x41 | CONFIG_LOCKBOX_FEATURE_INA219 | Power monitoring |
| CH422G | 0x58 | (always enabled) | GPIO expander (backlight, touch reset) |
| GT911 | 0x5D | (always enabled) | Touch controller |

---

## TROUBLESHOOTING

### Still Getting "defined but not used" Warnings?
- Ensure `idf.py fullclean` was run before build
- Verify all three changes in sdkconfig.defaults are correct
- Check that `sdkconfig` (generated) reflects the defaults

### I2C Device Not Found
- Verify device address in code matches hardware
- Check I2C bus with `i2c_scan` utility
- Ensure pull-up resistors present on SDA/SCL

### Servo Not Responding
- Requires `CONFIG_LOCKBOX_FEATURE_PCA9685=y`
- Check GPIO/PWM pins configured in `pca9685_device.c`
- Verify PCA9685 power supply

### Fingerprint/Face Not Recognizing Input
- R503: requires `CONFIG_LOCKBOX_FEATURE_R503=y`
- HLK-TX510: requires `CONFIG_LOCKBOX_FEATURE_HLK_TX510=y`
- Both require `CONFIG_LOCKBOX_FEATURE_SC16IS752=y`
- Check SC16IS752 UART baud rates match device expectations

### RTC Showing Wrong Date
- Requires `CONFIG_LOCKBOX_FEATURE_DS3231=y`
- May need RTC date set via admin screen
- Check DS3231 battery (if no power, RTC defaults to 2000-01-01)

---

## NEXT STEPS

1. **Immediate:** Apply Step 1 & 2 (disable mock mode, enable SC16IS752)
2. **Based on your hardware:** Apply Step 3 (enable specific features)
3. **Rebuild & test:** Run Step 4 and verify against checklist
4. **Debug if needed:** Check serial logs and troubleshooting section

---

## RELATED DOCUMENTATION

- [SC16IS752 Transport](main/comm/sc16is752_transport.c) - I2C→UART bridge
- [R503 Fingerprint](main/devices/r503_device.c) - Fingerprint sensor
- [HLK-TX510 Face](main/devices/hlk_tx510_device.c) - Face recognition
- [PCA9685 Servo](main/devices/pca9685_device.c) - Lock/servo control
- [DS3231 RTC](main/devices/ds3231_rtc.c) - Date/time persistence
- [INA219 Monitor](main/services/ina219_service.c) - Power monitoring
