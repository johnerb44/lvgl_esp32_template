/**
 * @file ui_screen_facescan.h
 */

#ifndef UI_SCREEN_FACESCAN_H
#define UI_SCREEN_FACESCAN_H

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
extern lv_obj_t *ui_screen_facescan;

// Screen creation function
void ui_screen_facescan_create(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_SCREEN_FACESCAN_H*/
