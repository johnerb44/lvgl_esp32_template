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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl_port.h"
#include "services/fingerprint_service.h"
#include "comm/sc16is752_transport.h"
#include <stdio.h>

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
static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_start_button = NULL;
static bool s_scan_in_progress = false;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static void fingerprint_scan_task(void *arg)
{
    (void)arg;
    ESP_LOGI(SCREEN_TAG, "Scan task: transport_ready=%d", sc16is752_transport_is_ready());
    sc16is752_transport_probe();

    fingerprint_match_result_t result = {0};
    esp_err_t err = fingerprint_service_match(&result);

    char msg[96];
    if (err != ESP_OK) {
        snprintf(msg, sizeof(msg), "Scan error: %s", esp_err_to_name(err));
    } else if (result.matched) {
        snprintf(msg, sizeof(msg), "Match: user %d (conf %u)", result.userid, result.confidence);
    } else {
        snprintf(msg, sizeof(msg), "No match (%s)", fingerprint_service_status_to_string(result.status));
    }

    if (lvgl_port_lock(1000)) {
        if (s_status_label && lv_obj_is_valid(s_status_label)) {
            lv_label_set_text(s_status_label, msg);
        }
        if (s_start_button && lv_obj_is_valid(s_start_button)) {
            lv_obj_clear_state(s_start_button, LV_STATE_DISABLED);
        }
        if (result.matched && ui_screen_get_pin) {
            lv_scr_load_anim(ui_screen_get_pin, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
        }
        lvgl_port_unlock();
    }

    s_scan_in_progress = false;
    vTaskDelete(NULL);
}

// Event handler for fingerprint button
static void fingerprint_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Fingerprint button clicked");
        if (s_scan_in_progress) {
            ESP_LOGW(SCREEN_TAG, "Scan already in progress");
            return;
        }
        s_scan_in_progress = true;

        if (lvgl_port_lock(1000)) {
            if (s_status_label && lv_obj_is_valid(s_status_label)) {
                lv_label_set_text(s_status_label, "Scanning...");
            }
            if (s_start_button && lv_obj_is_valid(s_start_button)) {
                lv_obj_add_state(s_start_button, LV_STATE_DISABLED);
            }
            lvgl_port_unlock();
        }

        BaseType_t created = xTaskCreate(fingerprint_scan_task,
                                         "fp_scan",
                                         4096,
                                         NULL,
                                         tskIDLE_PRIORITY + 2,
                                         NULL);
        if (created != pdPASS) {
            ESP_LOGE(SCREEN_TAG, "Failed to start fingerprint scan task");
            s_scan_in_progress = false;
            if (lvgl_port_lock(1000)) {
                if (s_status_label && lv_obj_is_valid(s_status_label)) {
                    lv_label_set_text(s_status_label, "Scan failed to start");
                }
                if (s_start_button && lv_obj_is_valid(s_start_button)) {
                    lv_obj_clear_state(s_start_button, LV_STATE_DISABLED);
                }
                lvgl_port_unlock();
            }
        }
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
    
    s_start_button = lv_button_create(ui_screen_fpscan);
    lv_obj_set_align(s_start_button, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(s_start_button, lv_color_hex(0xe19419), 0);
    lv_obj_set_y(s_start_button, 200);
    lv_obj_t * button_label = lv_label_create(s_start_button);
    lv_label_set_text(button_label, "Begin Scan");
    lv_obj_set_style_text_color(button_label, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(button_label, &lv_font_montserrat_26, 0);
    lv_obj_add_event_cb(s_start_button, fingerprint_btn_event_cb, LV_EVENT_CLICKED, NULL);

    s_status_label = lv_label_create(ui_screen_fpscan);
    lv_label_set_text(s_status_label, "Ready to scan");
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_24, 0);
    lv_obj_set_align(s_status_label, LV_ALIGN_CENTER);
    lv_obj_set_y(s_status_label, 280);
    
    ESP_LOGI(SCREEN_TAG, "Fingerprint scan screen created");

    LV_TRACE_OBJ_CREATE("finished");

}
