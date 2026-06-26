/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "waveshare_rgb_lcd_port.h"
#include "ch422g_driver.h"
#include "cJSON.h"
#include "sd/sd_card.h"
#include "ui/ui.h"
#include "user_store.h"
#include "user_mgmt_ui.h"
#include "app/lockbox_app.h"
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "driver/rtc_io.h"
#include "esp_system.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_chip_info.h"
// #include "esp_flash.h"


// Define APP_TAG for application logging
static const char *APP_TAG = "LVGL_TEMPLATE";

 /* Restart the board into UART download mode without a power cycle.
 *
 * GPIO0 is shared between the BOOT strapping pin and LCD RGB DATA6.  Once the
 * LCD panel initialises and actively drives DATA6 HIGH, the CH343P DTR signal
 * can no longer pull GPIO0 LOW during esptool's --before default_reset reset
 * sequence, blocking subsequent flashes (most visible on laptops whose USB
 * host controller has higher DTR/RTS latency than a desktop).
 *
 * This function:
 *  1. Reconfigures GPIO0 as a regular digital output driven LOW (overrides the
 *     LCD peripheral's I/O-MUX claim on the pin).
 *  2. Switches the pin to RTC GPIO mode and holds the LOW state via
 *     rtc_gpio_hold_en().  The RTC domain survives esp_restart() (a digital-
 *     domain / software reset), so GPIO0 remains LOW through the reset.
 *  3. Calls esp_restart().  The ROM bootloader samples GPIO0=LOW and enters
 *     UART download mode on UART0.
 *
 * esptool must be configured with --before no_reset (CONFIG_ESPTOOLPY_BEFORE=
 * "no_reset") so that it does NOT perform its own DTR/RTS reset sequence that
 * would toggle EN and trigger a hard reset, clearing the RTC GPIO hold.
 *
 * After flashing, esptool's --after hard_reset toggles EN (full chip reset),
 * which clears the RTC domain and the GPIO hold, allowing normal LCD operation
 * in the new firmware.
 */
static void enter_flash_mode(void)
{
    ESP_LOGI(APP_TAG, "Entering UART download mode — restarting with GPIO0 held LOW...");

    // Step 1: override LCD peripheral's mux claim; drive GPIO0 LOW as digital output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_NUM_0),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(GPIO_NUM_0, 0);

    // Step 2: switch to RTC GPIO and hold LOW through the upcoming soft reset
    rtc_gpio_init(GPIO_NUM_0);
    rtc_gpio_set_direction(GPIO_NUM_0, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(GPIO_NUM_0, 0);
    rtc_gpio_hold_en(GPIO_NUM_0);

    vTaskDelay(pdMS_TO_TICKS(200));  // allow log to flush before restart
    esp_restart();
}

static void flash_mode_btn_cb(lv_event_t *e)
{
    enter_flash_mode();
}

void app_main()
{
    // Initialize hardware
    waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB LCD 
    
    // Turn on backlight immediately after LCD init, before SD card
    ESP_LOGI(APP_TAG, "Turning on backlight");
    waveshare_rgb_lcd_bl_on();  // Turn on the screen backlight 
    vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to let backlight stabilize
    
    ESP_LOGI(APP_TAG, "Initializing SD card");
    esp_err_t sd_err = waveshare_sd_card_init();
    if (sd_err != ESP_OK) {
        ESP_LOGE(APP_TAG, "SD card init FAILED (%s) - file operations will not work", esp_err_to_name(sd_err));
    } else {
        ESP_LOGI(APP_TAG, "SD card mounted successfully");
        waveshare_sd_card_info();
        waveshare_sd_card_list_files();
    }

    // Initialize user store
    ESP_LOGI(APP_TAG, "Initializing user store");
    user_store_init(NULL); // Use default path

    ESP_LOGI(APP_TAG, "Initializing Secure Lockbox services");
    esp_err_t lockbox_err = lockbox_app_init();
    if (lockbox_err != ESP_OK) {
        ESP_LOGW(APP_TAG, "lockbox_app_init returned %s", esp_err_to_name(lockbox_err));
    }

    // End SD session - restore backlight (SD operations complete)
    ESP_LOGI(APP_TAG, "SD init complete, restoring backlight");
    ch422g_sd_card_enable(I2C_MASTER_NUM, false);
    
    // Small delay to allow LVGL task to fully start
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(APP_TAG, "LVGL 9.3 Template - Creating UI");
    
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(-1)) {
        // Initialize all UI screens
        ui_init();
        
        ESP_LOGI(APP_TAG, "Screen pointer before load: %p", (void*)ui_screen_change_pin);
        
        // Load the initial screen — always start at the main landing screen
        lv_scr_load(ui_screen_main);

        // Developer flash button: tap to restart into UART download mode.
        // Workflow: tap this button, then click Flash in VS Code.
        lv_obj_t *flash_btn = lv_btn_create(lv_scr_act());
        lv_obj_set_size(flash_btn, 90, 40);
        lv_obj_align(flash_btn, LV_ALIGN_BOTTOM_LEFT, 5, -5);
        lv_obj_t *flash_lbl = lv_label_create(flash_btn);
        lv_label_set_text(flash_lbl, "FLASH");
        lv_obj_center(flash_lbl);
        lv_obj_add_event_cb(flash_btn, flash_mode_btn_cb, LV_EVENT_CLICKED, NULL);
        
        ESP_LOGI(APP_TAG, "Screen loaded successfully");
        
        // Release the mutex
        lvgl_port_unlock();
    }
    
    ESP_LOGI(APP_TAG, "UI created successfully");
    ESP_LOGI(APP_TAG, "UI ready - showing main screen");
}
