/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Screen objects - exported for external access
extern lv_obj_t *ui_screen_start;
extern lv_obj_t *ui_screen_main;
extern lv_obj_t *ui_screen_biometric;
extern lv_obj_t *ui_screen_fpscan;
extern lv_obj_t *ui_screen_facescan;
extern lv_obj_t *ui_screen_get_pin;
extern lv_obj_t *ui_screen_change_pin;
extern lv_obj_t *ui_screen_home;
extern lv_obj_t *ui_screen_user_mgmt;


// UI initialization function
void ui_init(void);

// Screen creation functions
void ui_screen_start_create(void);
void ui_screen_main_create(void);
void ui_screen_biometric_create(void);
void ui_screen_fpscan_create(void);
void ui_screen_facescan_create(void);
void ui_screen_get_pin_create(void);
void ui_screen_change_pin_create(void);
void ui_screen_home_create(void);
void ui_screen_user_mgmt_create(void);
lv_obj_t *ui_screen_start_get(void);
lv_obj_t *ui_screen_main_get(void);
lv_obj_t *ui_screen_biometric_get(void);
lv_obj_t *ui_screen_fpscan_get(void);
lv_obj_t *ui_screen_facescan_get(void);
lv_obj_t *ui_screen_get_pin_get(void);
lv_obj_t *ui_screen_change_pin_get(void);
lv_obj_t *ui_screen_home_get(void);
lv_obj_t *ui_screen_user_mgmt_get(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // UI_H
