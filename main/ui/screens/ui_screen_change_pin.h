/**
 * @file ui_screen_change_pin.h
 */

#ifndef UI_SCREEN_CHANGE_PIN_H
#define UI_SCREEN_CHANGE_PIN_H

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
extern lv_obj_t *ui_screen_change_pin;

void ui_screen_change_pin_create(void);
void ui_screen_change_pin_set_context(int userid, bool forced);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_SCREEN_CHANGE_PIN_H*/