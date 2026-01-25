/**
 * @file ui_screen_home.h
 */

#ifndef UI_SCREEN_HOME_H
#define UI_SCREEN_HOME_H

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
extern lv_obj_t *ui_screen_home;


void ui_screen_home_create(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_SCREEN_HOME_H*/