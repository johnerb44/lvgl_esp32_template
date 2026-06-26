/**
 * @file ui_screen_main.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_screen_main.h"
#include "../ui.h"
#include "esp_log.h"
#include "ui/screens/ui_screen_get_pin.h"


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/
static const char *SCREEN_TAG = "UI_SCREEN_MAIN";
/***********************
 *  STATIC PROTOTYPES
 **********************/
static void test_btn_event_cb(lv_event_t *event);
static void first_time_setup_btn_event_cb(lv_event_t *event);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ui_screen_main_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static lv_style_t style_main;

    static bool style_inited = false;

    if (!style_inited) {
        lv_style_init(&style_main);
        lv_style_set_bg_color(&style_main, lv_color_hex(0x041d3a));
        lv_style_set_text_color(&style_main, lv_color_hex3(0xfff));

        style_inited = true;
    }

    ui_screen_main = lv_obj_create(NULL);
    lv_obj_set_style_text_color(ui_screen_main, lv_color_hex3(0xfff), 0);

    lv_obj_add_style(ui_screen_main, &style_main, 0);
    lv_obj_t * welcome_name = lv_label_create(ui_screen_main);
    lv_label_set_text(welcome_name, "Secure Lock Box");
    lv_obj_set_style_text_font(welcome_name, &lv_font_montserrat_28, 0);
    lv_obj_set_align(welcome_name, LV_ALIGN_TOP_MID);
    lv_obj_set_y(welcome_name, 10);

    lv_obj_add_style(ui_screen_main, &style_main, 0);
    lv_obj_t * info_box = lv_label_create(ui_screen_main);
    lv_obj_set_style_text_align(info_box, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(info_box, "M.O.S.S. has classified the contents of this box:\n\nTOP SECRET\n\nAccess requires biometric and PIN authentication.");
    lv_obj_set_style_text_font(info_box, &lv_font_montserrat_26, 0);
    lv_obj_align(info_box, LV_ALIGN_CENTER, 0, -30);
    
    lv_obj_t * start_button = lv_button_create(ui_screen_main);
    lv_obj_align(start_button, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(start_button, lv_color_hex(0xe19419), 0);
    
    lv_obj_t * lv_label_0 = lv_label_create(start_button);
    lv_label_set_text(lv_label_0, "Begin Authentication");
    lv_obj_set_style_text_color(lv_label_0, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_0, &lv_font_montserrat_22, 0);
    
    lv_obj_add_event_cb(start_button, test_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // First-Time Setup button
    lv_obj_t *setup_button = lv_button_create(ui_screen_main);
    lv_obj_align(setup_button, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(setup_button, lv_color_hex(0x197de0), 0);

    lv_obj_t *lv_label_1 = lv_label_create(setup_button);
    lv_label_set_text(lv_label_1, "First-Time Setup");
    lv_obj_set_style_text_color(lv_label_1, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_text_font(lv_label_1, &lv_font_montserrat_22, 0);

    lv_obj_add_event_cb(setup_button, first_time_setup_btn_event_cb, LV_EVENT_CLICKED, NULL);

    LV_TRACE_OBJ_CREATE("finished");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
// static void test_btn_event_cb(lv_event_t *event)
// {
//     if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
//         ESP_LOGI(SCREEN_TAG, "Main screen button clicked!");
//     }
// }

static void test_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Begin Authentication clicked");
        lv_scr_load_anim(ui_screen_biometric, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
    }
}

static void first_time_setup_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    ESP_LOGI(SCREEN_TAG, "First-Time Setup clicked");
    ui_screen_get_pin_set_auth_context(-1);
    ui_screen_get_pin_set_enroll_mode(true);
    lv_scr_load_anim(ui_screen_get_pin, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
}