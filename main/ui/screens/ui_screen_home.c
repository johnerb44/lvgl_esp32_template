/**
 * @file ui_screen_home.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_screen_home.h"
#include "../ui.h"
#include "esp_log.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/

/***********************
 *  STATIC PROTOTYPES
 **********************/
static const char *SCREEN_TAG = "UI_SCREEN_HOME";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ui_screen_home_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static bool style_inited = false;

    if (!style_inited) {
        
        style_inited = true;
    }

    ui_screen_home = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_screen_home, lv_color_hex(0x041d3a), 0);
    lv_obj_set_style_text_color(ui_screen_home, lv_color_hex3(0xfff), 0);

    lv_obj_t * title_label = lv_label_create(ui_screen_home);
    lv_label_set_text(title_label, "Secure Lock Box");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_30, 0);
    lv_obj_set_align(title_label, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title_label, 10);
    
    lv_obj_t * unlock_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(unlock_button, LV_ALIGN_CENTER);
    lv_obj_set_width(unlock_button, 420);
    lv_obj_set_y(unlock_button, -120);
    lv_obj_set_style_bg_color(unlock_button, lv_color_hex(0xe19419), 0);
    
    lv_obj_t * lv_label_0 = lv_label_create(unlock_button);
    lv_label_set_text(lv_label_0, "1  Unlock Secure Box");
    lv_obj_set_style_text_color(lv_label_0, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_0,  &lv_font_montserrat_26, 0);
    
    //lv_obj_add_screen_create_event(unlock_button, LV_EVENT_CLICKED, screen_biometric_select_create, LV_SCREEN_LOAD_ANIM_MOVE_TOP, 500, 0);
    
    lv_obj_t * change_pin_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(change_pin_button, LV_ALIGN_CENTER);
    lv_obj_set_width(change_pin_button, 420);
    lv_obj_set_y(change_pin_button, -50);
    lv_obj_set_style_bg_color(change_pin_button, lv_color_hex(0x197de0), 0);
    
    lv_obj_t * lv_label_1 = lv_label_create(change_pin_button);
    lv_label_set_text(lv_label_1, "2  Change PIN");
    lv_obj_set_style_text_color(lv_label_1, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_1,  &lv_font_montserrat_26, 0);
    
    //lv_obj_add_screen_create_event(change_pin_button, LV_EVENT_CLICKED, screen_biometric_select_create, LV_SCREEN_LOAD_ANIM_MOVE_TOP, 500, 0);
    
    lv_obj_t * register_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(register_button, LV_ALIGN_CENTER);
    lv_obj_set_width(register_button, 420);
    lv_obj_set_y(register_button, 20);
    lv_obj_set_style_bg_color(register_button, lv_color_hex(0x21e019), 0);
    
    lv_obj_t * lv_label_2 = lv_label_create(register_button);
    lv_label_set_text(lv_label_2, "3  Register Biometrics");
    lv_obj_set_style_text_color(lv_label_2, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_2, &lv_font_montserrat_26, 0);
    
    //lv_obj_add_screen_create_event(register_button, LV_EVENT_CLICKED, screen_biometric_select_create, LV_SCREEN_LOAD_ANIM_MOVE_TOP, 500, 0);
    
    lv_obj_t * admin_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(admin_button, LV_ALIGN_CENTER);
    lv_obj_set_width(admin_button, 420);
    lv_obj_set_y(admin_button, 90);
    lv_obj_set_style_bg_color(admin_button, lv_color_hex(0x8719e0), 0);
    
    lv_obj_t * lv_label_3 = lv_label_create(admin_button);
    lv_label_set_text(lv_label_3, "4  Admin");
    lv_obj_set_style_text_color(lv_label_3, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_3, &lv_font_montserrat_26, 0);
    
    //lv_obj_add_screen_create_event(admin_button, LV_EVENT_CLICKED, screen_biometric_select_create, LV_SCREEN_LOAD_ANIM_MOVE_TOP, 500, 0);
    
    lv_obj_t * signout_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(signout_button, LV_ALIGN_CENTER);
    lv_obj_set_width(signout_button, 420);
    lv_obj_set_y(signout_button, 160);
    lv_obj_set_style_bg_color(signout_button, lv_color_hex(0xd9e019), 0);
    
    lv_obj_t * lv_label_4 = lv_label_create(signout_button);
    lv_label_set_text(lv_label_4, "5  Sign Out/Lock Secure Box             ");
    lv_obj_set_style_text_color(lv_label_4, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_4, &lv_font_montserrat_26, 0);
    
    //lv_obj_add_screen_create_event(signout_button, LV_EVENT_CLICKED, screen_biometric_select_create, LV_SCREEN_LOAD_ANIM_MOVE_TOP, 500, 0);

    ESP_LOGI(SCREEN_TAG, "Home screen created");

    LV_TRACE_OBJ_CREATE("finished");

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

