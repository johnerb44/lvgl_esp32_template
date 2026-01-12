/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ui.h"
#include "screens/ui_screen_start.h"
#include "screens/ui_screen_main.h"
#include "esp_log.h"

static const char *UI_TAG = "UI";

// Screen objects
lv_obj_t *ui_screen_start = NULL;
lv_obj_t *ui_screen_main = NULL;

void ui_init(void)
{
    ESP_LOGI(UI_TAG, "Initializing UI");
    
    // Create all screens
    ui_screen_start_create();
    ui_screen_main_create();
    
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
