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
    
    lv_obj_t * start_button = lv_button_create(ui_screen_main);
    lv_obj_set_align(start_button, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(start_button, lv_color_hex(0xe19419), 0);
    lv_obj_t * lv_label_0 = lv_label_create(start_button);
    lv_label_set_text(lv_label_0, "Begin Authentication");
    lv_obj_set_style_text_color(lv_label_0, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_0, &lv_font_montserrat_22, 0);
    
    lv_obj_add_event_cb(start_button, test_btn_event_cb, LV_EVENT_CLICKED, NULL);

    LV_TRACE_OBJ_CREATE("finished");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void test_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Main screen button clicked!");
    }
}