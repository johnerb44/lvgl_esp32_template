/**
 * @file screen_biometric_select_gen.h
 */

#ifndef UI_SCREEN_BIOMETRIC_H
#define UI_SCREEN_BIOMETRIC_H

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
extern lv_obj_t *ui_screen_biometric;

// Screen creation function
void ui_screen_biometric_create(void);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*SCREEN_BIOMETRIC_H*/