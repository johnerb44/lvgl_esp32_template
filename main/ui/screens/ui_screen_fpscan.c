/**
 * @file screen_fingerprint_scan_gen.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_screen_fpscan.h"
#include "ui_screen_get_pin.h"
#include "../ui.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl_port.h"
#include "services/fingerprint_service.h"
#include "devices/r503_device.h"
#include "comm/sc16is752_transport.h"
#include "sdkconfig.h"
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
static lv_obj_t *s_enroll_button = NULL;
static bool s_scan_in_progress = false;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

static int s_matched_userid = -1;

static void navigate_to_pin_fp_cb(lv_timer_t *timer)
{
    lv_timer_del(timer);
    if (ui_screen_get_pin) {
        ui_screen_get_pin_set_auth_context(s_matched_userid);
        lv_scr_load_anim(ui_screen_get_pin, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
    }
}

static void fingerprint_scan_task(void *arg)
{
    (void)arg;
    ESP_LOGI(SCREEN_TAG, "Scan task: transport_ready=%d", sc16is752_transport_is_ready());
    sc16is752_transport_probe();

    fingerprint_match_result_t result = {0};
    esp_err_t err = fingerprint_service_match(&result);

    const char *msg;
    bool matched_with_user = false;
    if (err != ESP_OK) {
        msg = "Scan error";
    } else if (result.matched) {
        msg = "Fingerprint found - User match found";
        matched_with_user = true;
        s_matched_userid = result.userid;
        ESP_LOGI(SCREEN_TAG, "FP match: userid=%d confidence=%u", result.userid, result.confidence);
    } else if (result.status == R503_STATUS_OK) {
        // Device found a template but it's not linked to any user
        msg = "Fingerprint found - User match not found";
        ESP_LOGW(SCREEN_TAG, "FP match: template found but no linked user");
    } else if (result.status == R503_STATUS_NO_MATCH || result.status == R503_STATUS_SENSOR_ERROR) {
        msg = "Fingerprint not found - try another finger";
    } else {
        // NO_FINGER, TIMEOUT, COMM_ERROR
        msg = "Finger not detected - place finger on scanner";
    }

    if (lvgl_port_lock(1000)) {
        if (s_status_label && lv_obj_is_valid(s_status_label)) {
            lv_label_set_text(s_status_label, msg);
        }
        if (!matched_with_user && s_start_button && lv_obj_is_valid(s_start_button)) {
            lv_obj_clear_state(s_start_button, LV_STATE_DISABLED);
        }
        if (!matched_with_user && s_enroll_button && lv_obj_is_valid(s_enroll_button)) {
            lv_obj_clear_state(s_enroll_button, LV_STATE_DISABLED);
        }
        if (matched_with_user && ui_screen_get_pin) {
            // 5-second delay before navigating to PIN screen
            lv_timer_t *t = lv_timer_create(navigate_to_pin_fp_cb, 5000, NULL);
            lv_timer_set_repeat_count(t, 1);
        }
        lvgl_port_unlock();
    }

    s_scan_in_progress = false;
    vTaskDelete(NULL);
}

#if CONFIG_LOCKBOX_FEATURE_R503 && CONFIG_LOCKBOX_FEATURE_HLK_TX510
static void back_btn_event_cb_fp(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        lv_scr_load_anim(ui_screen_biometric, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 500, 0, false);
    }
}
#endif

static void fp_enroll_task(void *arg)
{
    (void)arg;

    int template_id = -1;
    r503_status_t status = R503_STATUS_SENSOR_ERROR;
    // Enroll without tying to a specific user — tests the sensor enrollment path
    esp_err_t err = r503_device_enroll(-1, &template_id, &status);

    char msg[128];
    if (err != ESP_OK) {
        snprintf(msg, sizeof(msg), "Enroll failed: %s", r503_status_to_string(status));
        ESP_LOGE(SCREEN_TAG, "%s", msg);
    } else if (status == R503_STATUS_DUPLICATE) {
        snprintf(msg, sizeof(msg), "Finger already enrolled");
        ESP_LOGW(SCREEN_TAG, "AutoEnroll rejected duplicate fingerprint");
    } else {
        snprintf(msg, sizeof(msg), "Enrolled! template_id=%d", template_id);
        ESP_LOGI(SCREEN_TAG, "%s", msg);
    }

    if (lvgl_port_lock(1000)) {
        if (s_status_label && lv_obj_is_valid(s_status_label)) {
            lv_label_set_text(s_status_label, msg);
        }
        if (s_enroll_button && lv_obj_is_valid(s_enroll_button)) {
            lv_obj_clear_state(s_enroll_button, LV_STATE_DISABLED);
        }
        if (s_start_button && lv_obj_is_valid(s_start_button)) {
            lv_obj_clear_state(s_start_button, LV_STATE_DISABLED);
        }
        lvgl_port_unlock();
    }

    s_scan_in_progress = false;
    vTaskDelete(NULL);
}

static void fp_enroll_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Enroll fingerprint button clicked");
        if (s_scan_in_progress) {
            ESP_LOGW(SCREEN_TAG, "Operation already in progress");
            return;
        }
        s_scan_in_progress = true;

        if (lvgl_port_lock(1000)) {
            if (s_status_label && lv_obj_is_valid(s_status_label)) {
                lv_label_set_text(s_status_label, "Enrolling fingerprint...");
            }
            if (s_enroll_button && lv_obj_is_valid(s_enroll_button)) {
                lv_obj_add_state(s_enroll_button, LV_STATE_DISABLED);
            }
            if (s_start_button && lv_obj_is_valid(s_start_button)) {
                lv_obj_add_state(s_start_button, LV_STATE_DISABLED);
            }
            lvgl_port_unlock();
        }

        BaseType_t created = xTaskCreate(fp_enroll_task, "fp_enroll", 4096,
                                         NULL, tskIDLE_PRIORITY + 2, NULL);
        if (created != pdPASS) {
            ESP_LOGE(SCREEN_TAG, "Failed to start enroll task");
            s_scan_in_progress = false;
            if (lvgl_port_lock(1000)) {
                if (s_status_label && lv_obj_is_valid(s_status_label)) {
                    lv_label_set_text(s_status_label, "Enroll failed to start");
                }
                if (s_enroll_button && lv_obj_is_valid(s_enroll_button)) {
                    lv_obj_clear_state(s_enroll_button, LV_STATE_DISABLED);
                }
                if (s_start_button && lv_obj_is_valid(s_start_button)) {
                    lv_obj_clear_state(s_start_button, LV_STATE_DISABLED);
                }
                lvgl_port_unlock();
            }
        }
    }
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

#if CONFIG_LOCKBOX_FEATURE_R503 && CONFIG_LOCKBOX_FEATURE_HLK_TX510
    lv_obj_t *back_btn = lv_button_create(ui_screen_fpscan);
    lv_obj_set_size(back_btn, 90, 40);
    lv_obj_set_align(back_btn, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(back_btn, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x444444), 0);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "< Back");
    lv_obj_set_style_text_color(back_label, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb_fp, LV_EVENT_CLICKED, NULL);
#endif

    lv_obj_t * lv_label_0 = lv_label_create(ui_screen_fpscan);
    lv_label_set_text(lv_label_0, "Fingerprint Scan");
    lv_obj_set_align(lv_label_0, LV_ALIGN_TOP_MID);
    lv_obj_set_y(lv_label_0, 40);
    lv_obj_set_style_text_font(lv_label_0, &lv_font_montserrat_28, 0);

        
    lv_obj_t * fp_instr_1 = lv_label_create(ui_screen_fpscan);
    lv_label_set_text(fp_instr_1, "1.   Fingerprint Scanner Ring is WHITE");
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
    lv_obj_set_pos(s_start_button, -100, 150);
    lv_obj_t * button_label = lv_label_create(s_start_button);
    lv_label_set_text(button_label, "Begin Scan");
    lv_obj_set_style_text_color(button_label, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(button_label, &lv_font_montserrat_26, 0);
    lv_obj_add_event_cb(s_start_button, fingerprint_btn_event_cb, LV_EVENT_CLICKED, NULL);

    s_enroll_button = lv_button_create(ui_screen_fpscan);
    lv_obj_set_align(s_enroll_button, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(s_enroll_button, lv_color_hex(0xe07019), 0);
    lv_obj_set_pos(s_enroll_button, 100, 150);
    lv_obj_t *enroll_label = lv_label_create(s_enroll_button);
    lv_label_set_text(enroll_label, "Enroll FP");
    lv_obj_set_style_text_color(enroll_label, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_text_font(enroll_label, &lv_font_montserrat_26, 0);
    lv_obj_add_event_cb(s_enroll_button, fp_enroll_btn_event_cb, LV_EVENT_CLICKED, NULL);

    s_status_label = lv_label_create(ui_screen_fpscan);
    lv_label_set_text(s_status_label, "Ready to scan");
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_24, 0);
    lv_obj_set_align(s_status_label, LV_ALIGN_CENTER);
    lv_obj_set_y(s_status_label, 230);
    
    ESP_LOGI(SCREEN_TAG, "Fingerprint scan screen created");

    LV_TRACE_OBJ_CREATE("finished");

}
