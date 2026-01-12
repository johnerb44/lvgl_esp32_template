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

// UI initialization function
void ui_init(void);

// Screen creation functions
void ui_screen_start_create(void);
void ui_screen_main_create(void);
lv_obj_t *ui_screen_start_get(void);
lv_obj_t *ui_screen_main_get(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // UI_H
