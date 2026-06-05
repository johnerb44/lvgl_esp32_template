/**
 * @file screen_biometric_select_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_screen_biometric.h"
#include "ui_screen_facescan.h"
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
static const char *SCREEN_TAG = "UI_SCREEN_BIOMETRIC";

// Event handler for back button
static void back_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        lv_scr_load_anim(ui_screen_main, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 500, 0, false);
    }
}

// Event handler for fingerprint button
static void fingerprint_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Fingerprint button clicked");
        lv_scr_load_anim(ui_screen_fpscan, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
    }
}

// Event handler for face scan button
static void face_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Face scan button clicked");
        lv_scr_load_anim(ui_screen_facescan, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
    }
}

/**********************
 *   GLOBAL FUNCTIONS
 *******************/
// void ui_screen_biometric_create(void)
// {
//     LV_TRACE_OBJ_CREATE("begin");


//     static bool style_inited = false;

//     if (!style_inited) {

//         style_inited = true;
//     }
    
//     ui_screen_biometric = lv_obj_create(NULL);

//     lv_obj_t *main_cont = lv_obj_create(ui_screen_biometric);
//     lv_obj_set_size(main_cont, LV_PCT(100), LV_PCT(100));
//     lv_obj_set_style_pad_all(main_cont, 0, 0);
    
//     // Create title label
//     lv_obj_t *title_label = lv_label_create(main_cont);
//     lv_label_set_text(title_label, "Select a Biometric Method");
//     lv_obj_set_style_text_font(title_label, &lv_font_montserrat_28, 0);
//     lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 40);

//     lv_obj_t * lv_obj_0 = lv_obj_create(NULL);
//     lv_obj_set_name_static(lv_obj_0, "screen_biometric_select_#");
//     lv_obj_set_style_bg_color(lv_obj_0, lv_color_hex(0x041d3a), 0);
//     lv_obj_set_style_text_color(lv_obj_0, lv_color_hex3(0xfff), 0);

//     lv_obj_t * lv_label_0 = lv_label_create(lv_obj_0);
//     lv_label_set_text(lv_label_0, "Select a biometric method:");
//     lv_obj_set_align(lv_label_0, LV_ALIGN_TOP_MID);
//     lv_obj_set_y(lv_label_0, 10);
//     lv_obj_set_style_text_font(lv_label_0, montserrat_30, 0);

//     lv_obj_t * lv_label_1 = lv_label_create(fingerprint_button);
//     lv_label_set_text(lv_label_1, "Fingerprint Scan");
//     lv_obj_center(lv_label_1);
//     lv_obj_set_style_text_color(lv_label_1, lv_color_hex3(0x000), 0);
    
//     lv_obj_t * face_button = lv_btn_create(ui_screen_biometric);
//     lv_obj_set_size(face_button, 260, 80);
//     lv_obj_align(face_button, LV_ALIGN_CENTER, 0, 40);
//     lv_obj_set_style_bg_color(face_button, lv_color_hex(0x19abe0), 0);
//     lv_obj_add_event_cb(face_button, face_btn_event_cb, LV_EVENT_CLICKED, NULL
//     lv_label_set_text(lv_label_0, "Select a biometric method:");
//     lv_obj_set_align(lv_label_0, LV_ALIGN_TOP_MID);
//     lv_obj_set_y(lv_label_0, 40);
//     lv_obj_set_style_text_font(lv_label_0, &lv_font_montserrat_28, 0);
    
//     lv_obj_t * fingerprint_button = lv_btn_create(ui_screen_biometric);
//     lv_obj_set_size(fingerprint_button, 260, 80);
//     lv_obj_align(fingerprint_button, LV_ALIGN_CENTER, 0, -60);
//     lv_obj_set_style_bg_color(fingerprint_button, lv_color_hex(0xe99309), 0);
//     lv_obj_t * lv_label_1 = lv_label_create(fingerprint_button);
//     lv_label_set_text(lv_label_1, "Fingerprint Scan");
//     lv_obj_center(lv_label_1);
//     lv_obj_set_style_text_color(lv_label_1, lv_color_hex3(0x000), 0);
//     lv_obj_add_event_cb(fingerprint_button, fingerprint_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
//     lv_obj_t * face_button = lv_btn_create(ui_screen_biometric);
//     lv_obj_set_size(face_button, 260, 80);
//     lv_obj_align(face_button, LV_ALIGN_CENTER, 0, 40);
//     lv_obj_set_style_bg_color(face_button, lv_color_hex(0x19abe0), 0);
//     lv_obj_t * lv_label_2 = lv_label_create(face_button);
//     lv_label_set_text(lv_label_2, "Face Scan");
//     lv_obj_center(lv_label_2);
//     lv_obj_set_style_text_color(lv_label_2, lv_color_hex3(0x000), 0);
//     lv_obj_add_event_cb(face_button, face_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
//     ESP_LOGI(SCREEN_TAG, "Biometric screen created");
// } 

/**********************
 *   STATIC FUNCTIONS
 **********************/

void ui_screen_biometric_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");


    static bool style_inited = false;

    if (!style_inited) {

        style_inited = true;
    }

    ui_screen_biometric = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_screen_biometric, lv_color_hex(0x041d3a), 0);
    lv_obj_set_style_text_color(ui_screen_biometric, lv_color_hex3(0xfff), 0);

    lv_obj_t * lv_label_0 = lv_label_create(ui_screen_biometric);
    lv_label_set_text(lv_label_0, "Select a biometric method:");
    lv_obj_set_align(lv_label_0, LV_ALIGN_TOP_MID);
    lv_obj_set_y(lv_label_0, 40);
    lv_obj_set_style_text_font(lv_label_0, &lv_font_montserrat_28, 0);
    
    lv_obj_t * fingerprint_button = lv_btn_create(ui_screen_biometric);
    lv_obj_set_size(fingerprint_button, 260, 80);
    lv_obj_set_align(fingerprint_button, LV_ALIGN_CENTER);
    lv_obj_set_y(fingerprint_button, -60);
    lv_obj_set_style_bg_color(fingerprint_button, lv_color_hex(0xe19419), 0);
    lv_obj_t * lv_label_1 = lv_label_create(fingerprint_button);
    lv_label_set_text(lv_label_1, "Fingerprint Scan");
    lv_obj_center(lv_label_1);
    lv_obj_set_style_text_color(lv_label_1, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_1, &lv_font_montserrat_20, 0);
    lv_obj_add_event_cb(fingerprint_button, fingerprint_btn_event_cb, LV_EVENT_CLICKED, NULL);
       
    lv_obj_t * face_button = lv_btn_create(ui_screen_biometric);
    lv_obj_set_size(face_button, 260, 80);
    lv_obj_set_align(face_button, LV_ALIGN_CENTER);
    lv_obj_set_y(face_button, 40);
    lv_obj_set_style_bg_color(face_button, lv_color_hex(0x197de0), 0);
    lv_obj_t * lv_label_2 = lv_label_create(face_button);
    lv_label_set_text(lv_label_2, "Face Scan");
    lv_obj_center(lv_label_2);
    lv_obj_set_style_text_color(lv_label_2, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_2, &lv_font_montserrat_20, 0);
    lv_obj_add_event_cb(face_button, face_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_btn = lv_btn_create(ui_screen_biometric);
    lv_obj_set_size(back_btn, 90, 40);
    lv_obj_set_align(back_btn, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(back_btn, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x444444), 0);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "< Back");
    lv_obj_set_style_text_color(back_label, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    ESP_LOGI(SCREEN_TAG, "Biometric screen created");

    LV_TRACE_OBJ_CREATE("finished");
}