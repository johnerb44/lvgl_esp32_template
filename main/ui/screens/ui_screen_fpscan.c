/**
 * @file screen_fingerprint_scan_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_screen_fpscan.h"
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
static const char *SCREEN_TAG = "UI_SCREEN_FPSCAN";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

 // Event handler for fingerprint button
static void fingerprint_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Fingerprint button clicked");
        lv_scr_load_anim(ui_screen_get_pin, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
    }
}


 void ui_screen_fpscan_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    static bool style_inited = false;

    if (!style_inited) {

        style_inited = true;
    }
    
    ui_screen_fpscan = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_screen_fpscan, lv_color_hex(0x041d3a), 0);
    lv_obj_set_style_text_color(ui_screen_fpscan, lv_color_hex3(0xfff), 0);

    lv_obj_t * lv_label_0 = lv_label_create(ui_screen_fpscan);
    lv_label_set_text(lv_label_0, "Fingerprint Scan");
    lv_obj_set_align(lv_label_0, LV_ALIGN_TOP_MID);
    lv_obj_set_y(lv_label_0, 40);
    lv_obj_set_style_text_font(lv_label_0, &lv_font_montserrat_28, 0);

        
    lv_obj_t * fp_instr_1 = lv_label_create(ui_screen_fpscan);
    lv_label_set_text(fp_instr_1, "1.   Fingerprint Scanner Ring is BLUE");
    lv_obj_set_style_text_font(fp_instr_1, &lv_font_montserrat_26, 0);
    lv_obj_set_align(fp_instr_1, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(fp_instr_1, 50);
    lv_obj_set_y(fp_instr_1, 100);
    
    lv_obj_t * fp_instr_2 = lv_label_create(ui_screen_fpscan);
    lv_label_set_text(fp_instr_2, "2.   Raise Protective Cover");
    lv_obj_set_style_text_font(fp_instr_2, &lv_font_montserrat_26, 0);
    lv_obj_set_align(fp_instr_2, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(fp_instr_2, 50);
    lv_obj_set_y(fp_instr_2, 150);
    
    lv_obj_t * fp_instr_3 = lv_label_create(ui_screen_fpscan);
    lv_label_set_text(fp_instr_3, "3.   Place Finger on Black Area of Scanner");
    lv_obj_set_style_text_font(fp_instr_3, &lv_font_montserrat_26, 0);
    lv_obj_set_align(fp_instr_3, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(fp_instr_3, 50);
    lv_obj_set_y(fp_instr_3, 200);
    
    lv_obj_t * start_button = lv_button_create(ui_screen_fpscan);
    lv_obj_set_align(start_button, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(start_button, lv_color_hex(0xe19419), 0);
    lv_obj_set_y(start_button, 200);
    lv_obj_t * button_label = lv_label_create(start_button);
    lv_label_set_text(button_label, "Begin Scan");
    lv_obj_set_style_text_color(button_label, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(button_label, &lv_font_montserrat_26, 0);
    lv_obj_add_event_cb(start_button, fingerprint_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    ESP_LOGI(SCREEN_TAG, "Fingerprint scan screen created");

    LV_TRACE_OBJ_CREATE("finished");

}
