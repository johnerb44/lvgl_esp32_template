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
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_chip_info.h"
// #include "esp_flash.h"


// Define APP_TAG for application logging
static const char *APP_TAG = "LVGL_TEMPLATE";

void app_main()
{
    // Initialize hardware
    waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB LCD 
    
    // Turn on backlight immediately after LCD init, before SD card
    ESP_LOGI(APP_TAG, "Turning on backlight");
    waveshare_rgb_lcd_bl_on();  // Turn on the screen backlight 
    vTaskDelay(pdMS_TO_TICKS(100)); // Small delay to let backlight stabilize
    
    printf("Initializing SD card...\n");
   
    waveshare_sd_card_init();
    printf("SD card initialized successfully!\n");
    printf("Reading SD card information...\n");
    waveshare_sd_card_info();
    printf("SD card information read successfully!\n");
    waveshare_sd_card_list_files();
    
    // Initialize user store
    ESP_LOGI(APP_TAG, "Initializing user store");
    user_store_init(NULL); // Use default path
    
    // End SD session - restore backlight (SD operations complete)
    printf("Ending SD session...\n");
    ch422g_sd_card_enable(I2C_MASTER_NUM, false);
    printf("Backlight restored!\n");
    
    // Small delay to allow LVGL task to fully start
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(APP_TAG, "LVGL 9.3 Template - Creating UI");
    
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(-1)) {
        // Initialize all UI screens
        ui_init();
        
        ESP_LOGI(APP_TAG, "Screen pointer before load: %p", (void*)ui_screen_change_pin);
        
        // Load the initial screen
        //lv_scr_load(ui_screen_main);
        //lv_scr_load(ui_screen_biometric);
        //lv_scr_load(ui_screen_start);
        //lv_scr_load(ui_screen_fpscan);
        //lv_scr_load(ui_screen_get_pin);
        //lv_scr_load(ui_screen_change_pin);
        // To load user management screen, uncomment:
        lv_scr_load(ui_screen_user_mgmt);
        user_mgmt_ui_show();  // Load user data and populate UI
        
        ESP_LOGI(APP_TAG, "Screen loaded successfully");
        
        // Release the mutex
        lvgl_port_unlock();
    }
    
    ESP_LOGI(APP_TAG, "UI created successfully");
    ESP_LOGI(APP_TAG, "User management available - navigate to ui_screen_user_mgmt");
}
