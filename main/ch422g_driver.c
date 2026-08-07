/*
 * CH422G GPIO Expander Driver Implementation
 * 
 * Provides thread-safe, centralized control of CH422G I2C GPIO expander
 * to prevent GPIO state conflicts between display, touch, and SD card peripherals.
 */

#include "ch422g_driver.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"

static const char *TAG = "ch422g_driver";

// Global state tracking
static uint8_t ch422g_gpio_state = 0x00;
static SemaphoreHandle_t ch422g_mutex = NULL;
static bool ch422g_initialized = false;

// I2C timeout for CH422G operations (in ticks)
#define CH422G_I2C_TIMEOUT_MS 1000
#define I2C_TIMEOUT_TICKS (CH422G_I2C_TIMEOUT_MS / portTICK_PERIOD_MS)

// GPIO pin for touch reset sequence (from waveshare_rgb_lcd_port.c)
#define GPIO_INPUT_IO_4 4

// Binary format macros for debugging
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0')

esp_err_t ch422g_init(i2c_port_t i2c_num)
{
    if (ch422g_initialized) {
        ESP_LOGW(TAG, "CH422G driver already initialized");
        return ESP_OK;
    }

    // Create mutex for thread-safe access
    ch422g_mutex = xSemaphoreCreateMutex();
    if (ch422g_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create CH422G mutex");
        return ESP_ERR_NO_MEM;
    }

    // Configure CH422G to output mode
    uint8_t control_cmd = 0x01;
    esp_err_t ret = i2c_master_write_to_device(i2c_num, CH422G_ADDR_CONTROL, 
                                               &control_cmd, 1, I2C_TIMEOUT_TICKS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure CH422G to output mode: %s", esp_err_to_name(ret));
        vSemaphoreDelete(ch422g_mutex);
        ch422g_mutex = NULL;
        return ret;
    }

    // USB_SEL (EXIO5/bit5) = 0 routes ESP32-S3 native USB to the USB-C connector
    // via FSUSB42UMX switch. USB_SEL=1 routes to CAN header (away from USB-C).
    // USB_SEL=0 is required for USB Serial/JTAG to enumerate on the USB-C port,
    // enabling flashing without the GPIO0/LCD DATA6 hardware conflict.
    ch422g_gpio_state = 0x00;  // 0b00000000 = all pins LOW (USB_SEL=0)
    
    ret = i2c_master_write_to_device(i2c_num, CH422G_ADDR_GPIO, 
                                     &ch422g_gpio_state, 1, I2C_TIMEOUT_TICKS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set initial GPIO state: %s", esp_err_to_name(ret));
        vSemaphoreDelete(ch422g_mutex);
        ch422g_mutex = NULL;
        return ret;
    }

    ch422g_initialized = true;
    ESP_LOGI(TAG, "CH422G driver initialized successfully (initial state: 0x%02X)", ch422g_gpio_state);
    return ESP_OK;
}

esp_err_t ch422g_write_pattern(i2c_port_t i2c_num, uint8_t pattern)
{
    ESP_LOGI(TAG, "Writing test pattern: 0x%02X (binary: " BYTE_TO_BINARY_PATTERN ")", 
             pattern, BYTE_TO_BINARY(pattern));
    return ch422g_write_gpio(i2c_num, pattern);
}

esp_err_t ch422g_set_pin(i2c_port_t i2c_num, uint8_t pin_mask, bool state)
{uint8_t old_state = ch422g_gpio_state;
    
    // Read-modify-write pattern: preserve unrelated pins
    if (state) {
        ch422g_gpio_state |= pin_mask;   // Set specified bits
    } else {
        ch422g_gpio_state &= ~pin_mask;  // Clear specified bits
    }

    ESP_LOGI(TAG, "set_pin: mask=0x%02X %s, 0x%02X -> 0x%02X (" BYTE_TO_BINARY_PATTERN " -> " BYTE_TO_BINARY_PATTERN ")",
             pin_mask, state ? "HIGH" : "LOW", old_state, ch422g_gpio_state,
             BYTE_TO_BINARY(old_state), BYTE_TO_BINARY(ch422g_gpio_state));
    // Take mutex for thread-safe operation
    if (xSemaphoreTake(ch422g_mutex, pdMS_TO_TICKS(CH422G_I2C_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire CH422G mutex");
        return ESP_ERR_TIMEOUT;
    }

    // Read-modify-write pattern: preserve unrelated pins
    if (state) {
        ch422g_gpio_state |= pin_mask;   // Set specified bits
    } else {
        ch422g_gpio_state &= ~pin_mask;  // Clear specified bits
    }

    // Write updated state to hardware
    esp_err_t ret = i2c_master_write_to_device(i2c_num, CH422G_ADDR_GPIO, 
                                               &ch422g_gpio_state, 1, I2C_TIMEOUT_TICKS);
    
    xSemaphoreGive(ch422g_mutex);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write CH422G GPIO state: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t ch422g_write_gpio(i2c_port_t i2c_num, uint8_t gpio_state)
{
    if (!ch422g_initialized || ch422g_mutex == NULL) {
        ESP_LOGE(TAG, "CH422G driver not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // Take mutex for thread-safe operation
    if (xSemaphoreTake(ch422g_mutex, pdMS_TO_TICKS(CH422G_I2C_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire CH422G mutex");
        return ESP_ERR_TIMEOUT;
    }

    // Direct write - overwrites all pins
    ch422g_gpio_state = gpio_state;
    
    // Protect I2C bus access with global mutex
    i2c_bus_lock("CH422G_write_gpio");
    esp_err_t ret = i2c_master_write_to_device(i2c_num, CH422G_ADDR_GPIO, 
                                               &ch422g_gpio_state, 1, I2C_TIMEOUT_TICKS);
    i2c_bus_unlock("CH422G_write_gpio");
    
    xSemaphoreGive(ch422g_mutex);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write CH422G GPIO state: %s", esp_err_to_name(ret));
    }

    return ret;
}

esp_err_t ch422g_touch_reset(i2c_port_t i2c_num)
{
    // Touch reset sequence using set_pin() to preserve USB_SEL and other unrelated pins
    // Original hardcoded sequence toggled EXIO1 (CTP_RST) while also manipulating
    // EXIO2, EXIO3, and EXIO5. We only need to control EXIO1 (CTP_RST).

    ESP_LOGI(TAG, "Performing touch controller reset sequence");

    // Configure GPIO4 as output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_INPUT_IO_4),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO4: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 1: GPIO4 = 1
    gpio_set_level(GPIO_INPUT_IO_4, 1);
    esp_rom_delay_us(100 * 1000);

    // Step 2: Set EXIO1 (CTP_RST) low - preserves other pins
    ret = ch422g_set_pin(i2c_num, CH422G_PIN_EXIO1, false);
    if (ret != ESP_OK) {
        return ret;
    }
    esp_rom_delay_us(100 * 1000);

    // Step 3: GPIO4 = 0
    gpio_set_level(GPIO_INPUT_IO_4, 0);
    esp_rom_delay_us(100 * 1000);

    // Step 4: Set EXIO1 (CTP_RST) high - preserves other pins
    ret = ch422g_set_pin(i2c_num, CH422G_PIN_EXIO1, true);
    if (ret != ESP_OK) {
        return ret;
    }
    esp_rom_delay_us(200 * 1000);

    ESP_LOGI(TAG, "Touch controller reset sequence completed");
    return ESP_OK;
}

esp_err_t ch422g_backlight_control(i2c_port_t i2c_num, bool enable)
{
    ESP_LOGI(TAG, "Backlight control: %s", enable ? "ON" : "OFF");

    // Backlight ON:  0x1E = EXIO1|EXIO2|EXIO3|EXIO4 (USB_SEL=0, native USB → USB-C)
    // Backlight OFF: 0x1A = EXIO1|EXIO3|EXIO4 (USB_SEL=0 preserved)
    uint8_t pattern = enable ? 0x1E : 0x1A;
    ESP_LOGI(TAG, "Writing backlight pattern: 0x%02X (" BYTE_TO_BINARY_PATTERN ")",
             pattern, BYTE_TO_BINARY(pattern));
    
    return ch422g_write_gpio(i2c_num, pattern);
}

esp_err_t ch422g_sd_card_enable(i2c_port_t i2c_num, bool enable)
{
    ESP_LOGI(TAG, "SD card access: %s", enable ? "ENABLED" : "DISABLED");

    if (!enable) {
        // Restore backlight when disabling SD card (USB_SEL=0 preserved)
        ESP_LOGI(TAG, "Restoring backlight after SD card access");
        return ch422g_write_gpio(i2c_num, 0x1E);  // Backlight ON, USB_SEL=0
    }
    
    // HARDWARE LIMITATION: SD card and backlight cannot be on simultaneously.
    // SD card pattern: 0x0A = EXIO1|EXIO3 (USB_SEL=0 preserved)
    uint8_t pattern = 0x0A;
    ESP_LOGI(TAG, "Writing SD pattern: 0x%02X (" BYTE_TO_BINARY_PATTERN ") - backlight will turn off temporarily",
             pattern, BYTE_TO_BINARY(pattern));

    return ch422g_write_gpio(i2c_num, pattern);
}
