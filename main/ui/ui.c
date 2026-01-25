/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ui.h"
#include "screens/ui_screen_start.h"
#include "screens/ui_screen_main.h"
#include "screens/ui_screen_biometric.h"
#include "screens/ui_screen_fpscan.h"
#include "screens/ui_screen_get_pin.h"
#include "screens/ui_screen_home.h"
#include "esp_log.h"

static const char *UI_TAG = "UI";

// Screen objects
lv_obj_t *ui_screen_start = NULL;
lv_obj_t *ui_screen_main = NULL;
lv_obj_t *ui_screen_biometric = NULL;
lv_obj_t *ui_screen_fpscan = NULL;
lv_obj_t *ui_screen_get_pin = NULL;
lv_obj_t *ui_screen_home = NULL;


void ui_init(void)
{
    ESP_LOGI(UI_TAG, "Initializing UI");
    
    // Create all screens
    ui_screen_start_create();
    ui_screen_main_create();
    ui_screen_biometric_create();
    ui_screen_fpscan_create();
    ui_screen_get_pin_create();
    ui_screen_home_create();
    ESP_LOGI(UI_TAG, "UI initialization complete");
}

lv_obj_t *ui_screen_start_get(void)
{
    return ui_screen_start;
}

lv_obj_t *ui_screen_main_get(void)
{
    return ui_screen_main;
}

lv_obj_t *ui_screen_biometric_get(void)
{
    return ui_screen_biometric;
}

lv_obj_t *ui_screen_fpscan_get(void)
{
    return ui_screen_fpscan;
}

lv_obj_t *ui_screen_get_pin_get(void)
{
    return ui_screen_get_pin;
}

lv_obj_t *ui_screen_home_get(void)
{
    return ui_screen_home;
}