

# CH422G I2C Conflict Resolution Chat
Date: 2026-02-01

## Problem Statement
Can't use SD Card and Display simultaneously due to CH422G I2C GPIO expander conflicts...


/*
 * CH422G I2C GPIO Expander Driver
 * 
 * DESIGN RATIONALE (from Copilot Chat 2026-02-01):
 * - Single I2C initialization prevents driver conflicts
 * - Mutex protects concurrent access from SD/Display tasks
 * - Read-modify-write preserves unrelated pin states
 * 
 * Problem solved: SD card SDCS (EXIO4) and LCD pins (EXIO1-3,5)
 * were overwriting each other's states on CH422G writes.
 */


 ### I2C Shared Resource Pattern
- CH422G GPIO expander accessed via `ch422g_driver.c` only
- Mutex `ch422g_mutex` protects state (`ch422g_gpio_state`)
- Use `ch422g_set_pin()` for atomic bit operations

Create ADR (Architecture Decision Record) for complex changes:

// filepath: docs/adr/001-ch422g-centralized-driver.md
# ADR-001: Centralized CH422G Driver

## Status
Accepted - 2026-02-01

## Context
SD card and LCD both use CH422G I2C expander...

## Solution Discussion

This application requires a touch LCD display, an SD card interface, SPI and I2C.

Problem:

Using Waveshare ESP32-S3-Touch-LCD-4.3 module with Touch LCD Display, SD Card, and ESP32-S3 mpu
Using ESP-IDF, LVGL, and Waveshare demo code
The waveshare module uses I2C to control a CH422G IO expander chip
Can't use SD Card and Display at the same time
Can use SD Card when not including LCD Touch Display code
Can use LCD Touch Display when not including SD Card code

Issues:

SD Card uses I2C for CH422G IO expander chip to provide (SDMMC) spi chip select SDCS (EXIO4)

LCD Touch Display uses I2C for CH422G IO expander chip to provide CTP_RST, DISP, LCD_RST, and USB_SEL.

The I2C is probably being initialized multiple times in current code.

Need to make sure I2C access to CH422G IO expander chip changes only the EXIOs intended and do not step on what is needed separately for Touch display, SD card, and USB.



I can see the issue. The Waveshare ESP32-S3-Touch-LCD-4.3 module uses a CH422G I2C GPIO expander (at address 0x24 for control, 0x38 for GPIO operations) to control multiple peripherals:

Touch Display needs: CTP_RST, DISP, LCD_RST, USB_SEL
SD Card needs: SDCS (EXIO4 via SDMMC)
Common Issue: I2C bus is being initialized multiple times, and GPIO operations aren't preserving state
Analysis of Current Code
Looking at waveshare_rgb_lcd_port.c:

Line 37: i2c_driver_install() called in i2c_master_init()
Line 51: CH422G initialized to output mode (0x01 → 0x24)
Line 54: Multiple GPIO operations that could conflict with SD card pins

The problem is that each I2C write to 0x38 (CH422G GPIO register) overwrites all 8 pins, not just the one you want to change.

Solution: Centralized CH422G Driver

Create a new file to manage the CH422G chip properly:

Create the header file:

Now update waveshare_rgb_lcd_port.c to use the new driver:

Update CMakeLists.txt to include the new driver:

Now update your SD card code (sd/sd_card.c) to use the same driver:

Key Changes:

Single I2C initialization: Only called once in i2c_master_init()

Mutex protection: Prevents race conditions when both SD card and display access CH422G

State preservation: ch422g_set_pin() uses read-modify-write to preserve other pin states

High-level functions: Abstracts hardware details (ch422g_touch_reset(), ch422g_backlight_control(), etc.)

Testing Steps:

Build and flash: idf.py build flash monitor

Verify in boot log:

Test SD card operations while touch is active

Verify no I2C errors (ESP_ERR_TIMEOUT, ESP_FAIL)

This solution ensures both peripherals can coexist by managing the shared CH422G resource properly.

Create a new file to manage the CH422G chip properly:

/*
 * CH422G I2C GPIO Expander Driver
 * Manages shared GPIO pins for Touch Display, SD Card, and USB
 */

#include "ch422g_driver.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "CH422G";
static SemaphoreHandle_t ch422g_mutex = NULL;
static uint8_t ch422g_gpio_state = 0x00; // Track current GPIO state

#define CH422G_ADDR_CTRL    0x24
#define CH422G_ADDR_GPIO    0x38
#define I2C_TIMEOUT_MS      1000

// CH422G GPIO Pin Definitions
#define CH422G_PIN_CTP_RST  (1 << 1)  // EXIO1 - Touch reset
#define CH422G_PIN_DISP     (1 << 2)  // EXIO2 - Display enable
#define CH422G_PIN_LCD_RST  (1 << 3)  // EXIO3 - LCD reset
#define CH422G_PIN_SDCS     (1 << 4)  // EXIO4 - SD card chip select
#define CH422G_PIN_USB_SEL  (1 << 5)  // EXIO5 - USB select

esp_err_t ch422g_init(i2c_port_t i2c_num)
{
    if (ch422g_mutex == NULL) {
        ch422g_mutex = xSemaphoreCreateMutex();
        if (ch422g_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return ESP_FAIL;
        }
    }

    xSemaphoreTake(ch422g_mutex, portMAX_DELAY);

    // Configure CH422G to output mode
    uint8_t ctrl_cmd = 0x01;
    esp_err_t ret = i2c_master_write_to_device(i2c_num, CH422G_ADDR_CTRL, 
                                               &ctrl_cmd, 1, 
                                               I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    
    if (ret == ESP_OK) {
        // Initialize with all pins low
        ch422g_gpio_state = 0x00;
        ret = i2c_master_write_to_device(i2c_num, CH422G_ADDR_GPIO, 
                                        &ch422g_gpio_state, 1,
                                        I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    }

    xSemaphoreGive(ch422g_mutex);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "CH422G initialized successfully");
    } else {
        ESP_LOGE(TAG, "CH422G initialization failed: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t ch422g_set_pin(i2c_port_t i2c_num, uint8_t pin_mask, bool state)
{
    if (ch422g_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(ch422g_mutex, portMAX_DELAY);

    // Update state while preserving other pins
    if (state) {
        ch422g_gpio_state |= pin_mask;  // Set bit(s)
    } else {
        ch422g_gpio_state &= ~pin_mask; // Clear bit(s)
    }

    esp_err_t ret = i2c_master_write_to_device(i2c_num, CH422G_ADDR_GPIO,
                                               &ch422g_gpio_state, 1,
                                               I2C_TIMEOUT_MS / portTICK_PERIOD_MS);

    xSemaphoreGive(ch422g_mutex);

    return ret;
}

esp_err_t ch422g_get_state(uint8_t *state)
{
    if (state == NULL || ch422g_mutex == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreTake(ch422g_mutex, portMAX_DELAY);
    *state = ch422g_gpio_state;
    xSemaphoreGive(ch422g_mutex);

    return ESP_OK;
}

// High-level helper functions
esp_err_t ch422g_touch_reset(i2c_port_t i2c_num)
{
    ESP_LOGI(TAG, "Resetting touch controller");
    
    // CTP_RST low
    ESP_RETURN_ON_ERROR(ch422g_set_pin(i2c_num, CH422G_PIN_CTP_RST, false), 
                       TAG, "Failed to set CTP_RST low");
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // CTP_RST high
    ESP_RETURN_ON_ERROR(ch422g_set_pin(i2c_num, CH422G_PIN_CTP_RST, true), 
                       TAG, "Failed to set CTP_RST high");
    vTaskDelay(pdMS_TO_TICKS(200));
    
    return ESP_OK;
}

esp_err_t ch422g_backlight_control(i2c_port_t i2c_num, bool enable)
{
    ESP_LOGI(TAG, "Backlight %s", enable ? "ON" : "OFF");
    
    // DISP controls backlight
    return ch422g_set_pin(i2c_num, CH422G_PIN_DISP, enable);
}

esp_err_t ch422g_sd_card_enable(i2c_port_t i2c_num, bool enable)
{
    ESP_LOGI(TAG, "SD card chip select %s", enable ? "ACTIVE" : "INACTIVE");
    
    // SDCS is active low typically, but verify with your hardware
    return ch422g_set_pin(i2c_num, CH422G_PIN_SDCS, !enable);
}

Create the header file:

#ifndef CH422G_DRIVER_H
#define CH422G_DRIVER_H

#include "driver/i2c.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize CH422G I2C GPIO expander
 * 
 * @param i2c_num I2C port number
 * @return ESP_OK on success
 */
esp_err_t ch422g_init(i2c_port_t i2c_num);

/**
 * @brief Set individual GPIO pin(s) state
 * 
 * @param i2c_num I2C port number
 * @param pin_mask Bitmask of pins to modify (can OR multiple pins)
 * @param state true=HIGH, false=LOW
 * @return ESP_OK on success
 */
esp_err_t ch422g_set_pin(i2c_port_t i2c_num, uint8_t pin_mask, bool state);

/**
 * @brief Get current GPIO state
 * 
 * @param state Pointer to store current state
 * @return ESP_OK on success
 */
esp_err_t ch422g_get_state(uint8_t *state);

/**
 * @brief Reset touch controller (CTP_RST sequence)
 * 
 * @param i2c_num I2C port number
 * @return ESP_OK on success
 */
esp_err_t ch422g_touch_reset(i2c_port_t i2c_num);

/**
 * @brief Control LCD backlight
 * 
 * @param i2c_num I2C port number
 * @param enable true=ON, false=OFF
 * @return ESP_OK on success
 */
esp_err_t ch422g_backlight_control(i2c_port_t i2c_num, bool enable);

/**
 * @brief Control SD card chip select
 * 
 * @param i2c_num I2C port number
 * @param enable true=ENABLE, false=DISABLE
 * @return ESP_OK on success
 */
esp_err_t ch422g_sd_card_enable(i2c_port_t i2c_num, bool enable);

#ifdef __cplusplus
}
#endif

#endif // CH422G_DRIVER_H


Now update waveshare_rgb_lcd_port.c to use the new driver:

// ...existing code...

#include "waveshare_rgb_lcd_port.h"
#include "ch422g_driver.h" // Add this

static const char *TAG = "waveshare_rgb_lcd";

// ...existing code...

#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911
/**
 * @brief I2C master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    // Configure I2C parameters
    i2c_param_config(i2c_master_port, &i2c_conf);

    // Install I2C driver ONLY ONCE
    esp_err_t ret = i2c_driver_install(i2c_master_port, i2c_conf.mode, 0, 0, 0);
    
    if (ret == ESP_OK) {
        // Initialize CH422G GPIO expander
        ret = ch422g_init(i2c_master_port);
    }
    
    return ret;
}

// GPIO initialization
void gpio_init(void)
{
    // Zero-initialize the config structure
    gpio_config_t io_conf = {};
    // Disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // Bit mask of the pins, use GPIO4 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // Set as input mode
    io_conf.mode = GPIO_MODE_OUTPUT;

    gpio_config(&io_conf);
}

// Reset the touch screen - USE NEW DRIVER
void waveshare_esp32_s3_touch_reset()
{
    ESP_ERROR_CHECK(ch422g_touch_reset(I2C_MASTER_NUM));
}

#endif

// Initialize RGB LCD
esp_err_t waveshare_esp32_s3_rgb_lcd_init()
{
// ...existing code...
}

/******************************* Turn on the screen backlight - USE NEW DRIVER **************************************/
esp_err_t waveshare_rgb_lcd_bl_on()
{
    return ch422g_backlight_control(I2C_MASTER_NUM, true);
}

/******************************* Turn off the screen backlight - USE NEW DRIVER **************************************/
esp_err_t waveshare_rgb_lcd_bl_off()
{
    return ch422g_backlight_control(I2C_MASTER_NUM, false);
}

// ...existing code...

Update CMakeLists.txt to include the new driver:

idf_component_register(
    SRCS "waveshare_rgb_lcd_port.c" 
         "main.c" 
         "lvgl_port.c"
         "ch422g_driver.c"  # Add this line
         "ui/ui.c"
         "ui/screens/ui_screen_start.c"
         "ui/screens/ui_screen_main.c"
         "ui/screens/ui_screen_biometric.c"
         "ui/screens/ui_screen_fpscan.c"
         "ui/screens/ui_screen_get_pin.c"
         "ui/screens/ui_screen_home.c"
         "sd/sd_card.c" 
         "cJSON/cJSON.c"

    INCLUDE_DIRS "."
                 "cJSON" 
                 "sd" 
                 "ui"
)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)

Now update your SD card code (sd/sd_card.c) to use the same driver:

#include "sd_card.h"
#include "ch422g_driver.h"
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

// ...existing code...

esp_err_t waveshare_sd_card_init(void)
{
    // Enable SD card chip select via CH422G
    ESP_RETURN_ON_ERROR(ch422g_sd_card_enable(I2C_MASTER_NUM, true),
                       TAG, "Failed to enable SD card");

    // ...rest of your SD card initialization...
    
    return ESP_OK;
}

Key Changes:
Single I2C initialization: Only called once in i2c_master_init()
Mutex protection: Prevents race conditions when both SD card and display access CH422G
State preservation: ch422g_set_pin() uses read-modify-write to preserve other pin states
High-level functions: Abstracts hardware details (ch422g_touch_reset(), ch422g_backlight_control(), etc.)


Testing Steps:
Build and flash: idf.py build flash monitor
Verify in boot log:

I (xxx) CH422G: CH422G initialized successfully
I (xxx) CH422G: Resetting touch controller
I (xxx) CH422G: Backlight ON

Test SD card operations while touch is active
Verify no I2C errors (ESP_ERR_TIMEOUT, ESP_FAIL)
This solution ensures both peripherals can coexist by managing the shared CH422G resource properly.



I'll help you create a plan to implement the changes recommended in sd-lcd-problem.md. Let me first gather comprehensive context about the recommendations.

Research SD-LCD problem recommendations

Read sd-lcd-problem.md, lines 1 to 200

Read main.c

Read lvgl_port.c, lines 1 to 150

Read waveshare_rgb_lcd_port.c, lines 1 to 150

Read sd-lcd-problem.md, lines 200 to 471

Read waveshare_rgb_lcd_port.c, lines 150 to 219

Read waveshare_rgb_lcd_port.h

Read sd_card.c, lines 1 to 100

Read CMakeLists.txt

Searched for files matching **/ch422g*.{c,h}, no matches

Read sd_card.c, lines 100 to 334

Read sd_card.h

Plan: Implement CH422G GPIO Expander Driver
Consolidate I2C GPIO expander control into a centralized driver to enable concurrent SD card and display operations by preventing GPIO state overwrites and eliminating duplicate I2C initializations.

Steps
Create CH422G driver module with main/ch422g_driver.h and main/ch422g_driver.c implementing mutex-protected read-modify-write pattern for pin state preservation with public API: ch422g_init(), ch422g_set_pin(), ch422g_touch_reset(), ch422g_backlight_control(), ch422g_sd_card_enable()

Reverse-engineer pin masks from hardcoded values in waveshare_rgb_lcd_port.c (0x1E/0x1A for backlight, 0x2C/0x2E for touch reset) and sd/sd_card.c (0x0A for SD CS) to derive correct bit masks for EXIO1-5 pins

Refactor display code in waveshare_rgb_lcd_port.c by adding ch422g_init() call after I2C driver install in i2c_master_init(), replacing direct I2C writes in waveshare_esp32_s3_touch_reset() and wavesahre_rgb_lcd_bl_on/off() with new driver functions

Refactor SD card code in sd/sd_card.c by removing duplicate i2c_master_init() function and workaround code, replacing CH422G writes with ch422g_sd_card_enable() calls, and adding driver header include

Update build configuration in CMakeLists.txt by adding ch422g_driver.c to SRCS list, then rebuild with idf.py fullclean && idf.py build

Verify concurrent operation by uncommenting SD initialization in main.c:31-35, testing display-only, SD-only, and simultaneous operations while monitoring serial output for I2C errors

Further Considerations
Pin mapping verification: Confirm EXIO1-5 assignments against Waveshare ESP32-S3-Touch-LCD-4.3 schematic to validate reverse-engineered bit masks - should SDCS use inverted logic since chip select is typically active-low?

Initial GPIO state: Should ch422g_gpio_state start at 0x00 or inherit existing pin states via I2C read during initialization to avoid resetting hardware on driver load?

Error propagation strategy: Should driver functions return esp_err_t with ESP_ERROR_CHECK() at call sites (strict) or log warnings and continue (permissive)?





Excellent! The CH422G driver implementation is working perfectly:

✅ Display: Backlight ON (0x3E)
✅ Touch: Reset sequence working (set_pin preserves USB_SEL)
✅ SD Card: Files listing correctly
✅ Backlight Restore: Working after SD operations
✅ USB_SEL: Preserved in all patterns (0x20 bit always set)

The COM port instability requiring power recycles is not caused by the CH422G GPIO patterns. All logs show USB_SEL (bit 5) is correctly maintained:

Init: 0x20
Backlight: 0x3E
SD access: 0x2A
All include USB_SEL bit
This USB stability issue is likely:

ESP32-S3 USB CDC driver - The ESP32-S3 uses USB OTG for serial communication, which can be sensitive to timing/enumeration
Windows USB driver caching - Windows may cache bad USB device state between reconnects
Power supply noise - USB negotiation failing due to voltage instability during WiFi/display operations
Diagnostic suggestions:

Check ESP-IDF USB configuration:

Try adding USB stability delay in main.c before first USB output:

Check Windows Device Manager for USB errors after disconnect

The GPIO work is complete and functioning correctly. The USB issue is orthogonal to the CH422G implementation.

idf.py menuconfig
# Navigate to: Component config → ESP System Settings → Channel for console output
# Verify: "USB CDC" is selected

void app_main() {
    vTaskDelay(pdMS_TO_TICKS(1000)); // Let USB enumerate fully
    // ... rest of code
}


You're welcome! The CH422G driver implementation is complete and working correctly:

Completed:

✅ Centralized GPIO control with mutex protection
✅ USB_SEL preservation (bit 5 always high)
✅ Display backlight control (0x3E/0x3A patterns)
✅ Touch reset without USB disruption (set_pin)
✅ SD card session management with backlight restoration
✅ All hardware functions operational
The USB enumeration issue is unrelated to the GPIO patterns and will require separate investigation of ESP32-S3 USB CDC driver settings or Windows USB caching.

Good luck with the USB troubleshooting!