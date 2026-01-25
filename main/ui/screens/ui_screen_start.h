/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef UI_SCREEN_START_H
#define UI_SCREEN_START_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Screen object (declared in ui.c)
extern lv_obj_t *ui_screen_start;

// Screen creation function
void ui_screen_start_create(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif // UI_SCREEN_START_H
