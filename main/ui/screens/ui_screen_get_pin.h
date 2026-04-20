/**
 * @file ui_screen_get_pin.h
 */

#ifndef UI_SCREEN_GET_PIN_H
#define UI_SCREEN_GET_PIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include "lvgl.h"


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

// Screen object (declared in ui.c)
extern lv_obj_t *ui_screen_get_pin;

void ui_screen_get_pin_create(void);
void ui_screen_get_pin_set_auth_context(int userid);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_SCREEN_GET_PIN_H*/