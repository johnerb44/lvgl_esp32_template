/**
 * @file ui_screen_fpscan.h
 */

#ifndef UI_SCREEN_FPSCAN_H
#define UI_SCREEN_FPSCAN_H

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
extern lv_obj_t *ui_screen_fpscan;

// Screen creation function
void ui_screen_fpscan_create(void);


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_SCREEN_FPSCAN_H*/