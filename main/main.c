/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "waveshare_rgb_lcd_port.h"
#include "ui/ui.h"

// Define APP_TAG for application logging
static const char *APP_TAG = "LVGL_TEMPLATE";

void app_main()
{
    // Initialize hardware
    waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB LCD 
    waveshare_rgb_lcd_bl_on();  // Turn on the screen backlight 
    
    ESP_LOGI(APP_TAG, "LVGL 9.3 Template - Creating UI");
    
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(-1)) {
        // Initialize all UI screens
        ui_init();
        
        // Load the initial screen
        lv_scr_load(ui_screen_start);
        
        // Release the mutex
        lvgl_port_unlock();
    }
    
    ESP_LOGI(APP_TAG, "UI created successfully");
}
