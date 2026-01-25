/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ui_screen_start.h"
#include "../ui.h"
#include "esp_log.h"

static const char *SCREEN_TAG = "UI_SCREEN_START";

// Event handler for test button
static void test_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Button clicked! LVGL 9.3 template working correctly.");
    }
}

// Create the start screen
void ui_screen_start_create(void)
{
    ui_screen_start = lv_obj_create(NULL);
    
    lv_obj_t *main_cont = lv_obj_create(ui_screen_start);
    lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(main_cont, 0, 0);
    
    // Create title label
    lv_obj_t *title_label = lv_label_create(main_cont);
    lv_label_set_text(title_label, "LVGL 9.3 Template");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_28, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 40);
    
    // Create info label
    lv_obj_t *info_label = lv_label_create(main_cont);
    lv_label_set_text(info_label, "ESP32-S3 + Waveshare 4.3\" LCD\n"
                                  "800x480 RGB Display\n"
                                  "GT911 Touch Controller");
    lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(info_label, LV_ALIGN_CENTER, 0, -20);
    
    // Create test button
    lv_obj_t *btn = lv_btn_create(main_cont);
    lv_obj_set_size(btn, 180, 60);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_add_event_cb(btn, test_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Test Button");
    lv_obj_center(btn_label);
    
    ESP_LOGI(SCREEN_TAG, "Start screen created");
}
