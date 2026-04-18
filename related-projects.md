# Related Projects

These projects are proof-of-concept codebases for Secure Lockbox features.

Each project currently builds and runs on its own, but may not fully meet final system requirements.

Code from these projects should be treated as reference input for integration into the final Secure Lockbox application, not as mandatory one-to-one implementations.

## Projects

### `lvgl_esp32_template`

- **Folder:** `/home/erbj/lvgl-projects/lvgl_esp32_template`
- **Role:** Core Secure Lockbox application.
- **Details:**
  - Provides the LVGL (v9.3) GUI foundation.
  - Other feature modules are expected to integrate into this codebase.
  - Written in C on ESP-IDF v5.1.6.
  - Targets a single ESP32-S3 on the Waveshare ESP32-S3-Touch-LCD-4.3 module.

### `i2c_tools_2`

- **Folder:** `/home/erbj/lvgl-projects/i2c_tools_2`
- **Role:** I2C communication and peripheral integration testbed.
- **Details:**
  - Implements baseline I2C utilities.
  - Communicates with the SC16IS752 bridge for serial access to:
    - R503 fingerprint module
    - HLK-TX510 face recognition module
    - Expanded GPIO status signals (lid, module stow state, fingerprint touch wake)
  - Communicates with the PCA9685 PWM/servo driver for lock actuation.
  - Communicates with the DS3231 RTC module.
  - Includes command-level testing for HLK-TX510 and R503 interactions.

### `uart_echo_1`

- **Folder:** `/home/erbj/lvgl-projects/uart_echo_1`
- **Role:** UART-level R503 fingerprint feature test project.

### `hlk_tx510`

- **Folder:** `/home/erbj/lvgl-projects/hlk_tx510`
- **Role:** HLK-TX510 face recognition feature test project.
- **Current status:**
  - Not currently working in its present form.
  - Built for direct ESP32-S3 UART usage rather than SC16IS752 TX/RX paths.
  - Needs refactoring to align with the `i2c_tools_2` communication approach.

### `servo_driver`

- **Folder:** `/home/erbj/lvgl-projects/servo_driver`
- **Role:** PCA9685 PWM/servo lock mechanism test project.
- **Details:**
  - Covers lock servo control for locked/unlocked positions.
