/**
 * @file ui_screen_main.h
 */

#ifndef UI_SCREEN_MAIN_H
#define UI_SCREEN_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

 #include "lvgl.h"


// #ifdef LV_LVGL_H_INCLUDE_SIMPLE
//     #include "lvgl.h"
// #else
//     #include "lvgl/lvgl.h"
// #endif

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
extern lv_obj_t *ui_screen_main;

// Screen object (declared in ui.c)
//extern lv_obj_t *ui_screen_start;

// Screen creation function
//void ui_screen_main_create(void);
void ui_screen_main_create(void);



//lv_obj_t * screen_start_create(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*UI_SCREEN_MAIN_H*/