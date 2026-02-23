/*
 * CH422G GPIO Expander Driver
 * 
 * This driver provides centralized control of the CH422G I2C GPIO expander chip
 * used on the Waveshare ESP32-S3-Touch-LCD-4.3 module. It prevents GPIO state
 * conflicts when multiple peripherals (display, touch, SD card) need simultaneous
 * access to the shared CH422G pins.
 * 
 * I2C Addresses:
 *   - 0x24: Control register (set output mode)
 *   - 0x38: GPIO operations (read/write pin states)
 * 
 * Pin Assignments (EXIO0-7):
 *   - EXIO1 (bit 1): CTP_RST (Touch reset)
 *   - EXIO2 (bit 2): DISP (Display enable)
 *   - EXIO3 (bit 3): LCD_RST (LCD reset)
 *   - EXIO4 (bit 4): SDCS (SD card chip select)
 *   - EXIO5 (bit 5): USB_SEL (USB selection)
 */

#ifndef CH422G_DRIVER_H
#define CH422G_DRIVER_H

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

// CH422G I2C addresses
#define CH422G_ADDR_CONTROL 0x24  // Control register address
#define CH422G_ADDR_GPIO    0x38  // GPIO register address

// CH422G pin masks (based on reverse-engineered values from existing code)
// Backlight ON: 0x1E = 0b00011110 = EXIO1|EXIO2|EXIO3|EXIO4
// Backlight OFF: 0x1A = 0b00011010 = EXIO1|EXIO3|EXIO4
// Touch reset: 0x2C = 0b00101100 = EXIO2|EXIO3|EXIO5, then 0x2E = 0b00101110 = EXIO1|EXIO2|EXIO3|EXIO5
// SD card: 0x0A = 0b00001010 = EXIO1|EXIO3
#define CH422G_PIN_EXIO1    (1 << 1)  // Touch reset (CTP_RST)
#define CH422G_PIN_EXIO2    (1 << 2)  // Display enable (DISP) / Backlight control
#define CH422G_PIN_EXIO3    (1 << 3)  // LCD reset (LCD_RST)
#define CH422G_PIN_EXIO4    (1 << 4)  // SD card chip select (SDCS) / Backlight control
#define CH422G_PIN_EXIO5    (1 << 5)  // USB selection (USB_SEL)

// Composite masks for common operations
#define CH422G_MASK_BACKLIGHT_ON   (CH422G_PIN_EXIO1 | CH422G_PIN_EXIO2 | CH422G_PIN_EXIO3 | CH422G_PIN_EXIO4)
#define CH422G_MASK_BACKLIGHT_OFF  (CH422G_PIN_EXIO1 | CH422G_PIN_EXIO3 | CH422G_PIN_EXIO4)
#define CH422G_MASK_TOUCH_RESET_0  (CH422G_PIN_EXIO2 | CH422G_PIN_EXIO3 | CH422G_PIN_EXIO5)
#define CH422G_MASK_TOUCH_RESET_1  (CH422G_PIN_EXIO1 | CH422G_PIN_EXIO2 | CH422G_PIN_EXIO3 | CH422G_PIN_EXIO5)
#define CH422G_MASK_SD_CARD        (CH422G_PIN_EXIO1 | CH422G_PIN_EXIO3)

/**
 * @brief Initialize the CH422G driver
 * 
 * Sets up mutex for thread-safe operation and configures CH422G to output mode.
 * Must be called after I2C driver is installed but before any peripheral operations.
 * 
 * @param i2c_num I2C port number
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ch422g_init(i2c_port_t i2c_num);

/**
 * @brief Set or clear specific CH422G GPIO pins while preserving others
 * 
 * Uses read-modify-write pattern to change only specified pins without
 * affecting unrelated GPIO states. Thread-safe via internal mutex.
 * 
 * @param i2c_num I2C port number
 * @param pin_mask Bit mask of pins to modify (use CH422G_PIN_* defines)
 * @param state true to set pins high, false to set pins low
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ch422g_set_pin(i2c_port_t i2c_num, uint8_t pin_mask, bool state);

/**
 * @brief Write complete GPIO state to CH422G (overwrites all pins)
 * 
 * Directly writes 8-bit value to GPIO register. Use sparingly - prefer
 * ch422g_set_pin() to preserve unrelated pin states.
 * 
 * @param i2c_num I2C port number
 * @param gpio_state 8-bit value to write to GPIO register
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ch422g_write_gpio(i2c_port_t i2c_num, uint8_t gpio_state);

/**
 * @brief Perform touch controller reset sequence
 * 
 * Executes the GT911 touch reset sequence using CH422G EXIO1 (CTP_RST)
 * and GPIO4 with proper timing delays.
 * 
 * @param i2c_num I2C port number
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ch422g_touch_reset(i2c_port_t i2c_num);

/**
 * @brief Control display backlight
 * 
 * Turns backlight on/off by setting appropriate EXIO pins while
 * preserving other GPIO states.
 * 
 * @param i2c_num I2C port number
 * @param enable true to turn backlight on, false to turn off
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ch422g_backlight_control(i2c_port_t i2c_num, bool enable);

/**
 * @brief Control SD card chip select
 * 
 * Enables/disables SD card access by setting EXIO4 (SDCS) while
 * preserving other GPIO states.
 * 
 * @param i2c_num I2C port number
 * @param enable true to enable SD card access, false to disable
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ch422g_sd_card_enable(i2c_port_t i2c_num, bool enable);

/**
 * @brief Write test pattern to CH422G for debugging
 * 
 * Directly writes a pattern to test GPIO behavior. Use for diagnostics only.
 * 
 * @param i2c_num I2C port number
 * @param pattern 8-bit pattern to write
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ch422g_write_pattern(i2c_port_t i2c_num, uint8_t pattern);

#ifdef __cplusplus
}
#endif

#endif // CH422G_DRIVER_H
