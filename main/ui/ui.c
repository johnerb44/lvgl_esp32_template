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
#include "screens/ui_screen_facescan.h"
#include "screens/ui_screen_get_pin.h"
#include "screens/ui_screen_change_pin.h"
#include "screens/ui_screen_home.h"
#include "screens/ui_screen_enroll.h"
#include "screens/ui_screen_fp_manage.h"
#include "screens/ui_screen_rtc_date.h"
#include "user_mgmt_ui.h"
#include "esp_log.h"

static const char *UI_TAG = "UI";

// Screen objects
lv_obj_t *ui_screen_start = NULL;
lv_obj_t *ui_screen_main = NULL;
lv_obj_t *ui_screen_biometric = NULL;
lv_obj_t *ui_screen_fpscan = NULL;
lv_obj_t *ui_screen_facescan = NULL;
lv_obj_t *ui_screen_get_pin = NULL;
lv_obj_t *ui_screen_change_pin = NULL;
lv_obj_t *ui_screen_home = NULL;
lv_obj_t *ui_screen_user_mgmt = NULL;
lv_obj_t *ui_screen_enroll = NULL;
lv_obj_t *ui_screen_fp_manage = NULL;
lv_obj_t *ui_screen_rtc_date = NULL;


void ui_init(void)
{
    ESP_LOGI(UI_TAG, "Initializing UI");
    
    // Create all screens
    ui_screen_start_create();
    ui_screen_main_create();
    ui_screen_biometric_create();
    ui_screen_fpscan_create();
    ui_screen_facescan_create();
    ui_screen_get_pin_create();
    ui_screen_change_pin_create();
    ui_screen_home_create();
    ui_screen_user_mgmt_create();
    ui_screen_enroll_create();
    ui_screen_fp_manage_create();
    /* RTC screen created on-demand from admin button, not at init */
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

lv_obj_t *ui_screen_facescan_get(void)
{
    return ui_screen_facescan;
}

lv_obj_t *ui_screen_get_pin_get(void)
{
    return ui_screen_get_pin;
}

lv_obj_t *ui_screen_home_get(void)
{
    return ui_screen_home;
}

lv_obj_t *ui_screen_change_pin_get(void)
{
    return ui_screen_change_pin;
}

void ui_screen_user_mgmt_create(void)
{
    if (!ui_screen_user_mgmt) {
        ui_screen_user_mgmt = user_mgmt_ui_create(0); // userid set at show time from session
    }
}

lv_obj_t *ui_screen_user_mgmt_get(void)
{
    return ui_screen_user_mgmt;
}