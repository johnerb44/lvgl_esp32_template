/**
 * @file ui_screen_facescan.c
 * @brief Face scan screen for HLK-TX510 face recognition
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_screen_facescan.h"
#include "ui_screen_get_pin.h"
#include "../ui.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl_port.h"
#include "services/face_service.h"
#include "devices/hlk_tx510_device.h"
#include "sdkconfig.h"
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/

static const char *SCREEN_TAG = "UI_SCREEN_FACESCAN";
static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_start_button = NULL;
static lv_obj_t *s_enroll_button = NULL;
static bool s_scan_in_progress = false;

#if CONFIG_LOCKBOX_FEATURE_R503 && CONFIG_LOCKBOX_FEATURE_HLK_TX510
static void back_btn_event_cb_face(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        lv_scr_load_anim(ui_screen_biometric, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 500, 0, false);
    }
}
#endif

/**********************
 *  STATIC FUNCTIONS
 **********************/

static void face_scan_task(void *arg)
{
    (void)arg;

    face_match_result_t result = {0};
    esp_err_t err = face_service_match(&result);

    char msg[128];
    if (err != ESP_OK) {
        snprintf(msg, sizeof(msg), "Scan error: %s", esp_err_to_name(err));
    } else if (result.matched) {
        snprintf(msg, sizeof(msg), "Face matched! (user %d)", result.userid);
    } else {
        switch (result.status) {
            case HLK_TX510_STATUS_NO_FACE:
                snprintf(msg, sizeof(msg), "No face detected. Please look at camera.");
                break;
            case HLK_TX510_STATUS_POSE_ERROR:
                snprintf(msg, sizeof(msg), "Face angle too large. Face forward.");
                break;
            case HLK_TX510_STATUS_2D_LIVENESS:
            case HLK_TX510_STATUS_3D_LIVENESS:
                snprintf(msg, sizeof(msg), "Liveness check failed. Please try again.");
                break;
            case HLK_TX510_STATUS_NO_MATCH:
                snprintf(msg, sizeof(msg), "Face not recognized.");
                break;
            default:
                snprintf(msg, sizeof(msg), "No match (%s)",
                         face_service_status_to_string(result.status));
                break;
        }
    }

    if (lvgl_port_lock(1000)) {
        if (s_status_label && lv_obj_is_valid(s_status_label)) {
            lv_label_set_text(s_status_label, msg);
        }
        if (s_start_button && lv_obj_is_valid(s_start_button)) {
            lv_obj_clear_state(s_start_button, LV_STATE_DISABLED);
        }
        if (result.matched && ui_screen_get_pin) {
            ui_screen_get_pin_set_auth_context(result.userid);
            lv_scr_load_anim(ui_screen_get_pin, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
        }
        lvgl_port_unlock();
    }

    s_scan_in_progress = false;
    vTaskDelete(NULL);
}

static void face_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Face scan button clicked");
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

        BaseType_t created = xTaskCreate(face_scan_task,
                                         "face_scan",
                                         4096,
                                         NULL,
                                         tskIDLE_PRIORITY + 2,
                                         NULL);
        if (created != pdPASS) {
            ESP_LOGE(SCREEN_TAG, "Failed to start face scan task");
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

static void face_enroll_task(void *arg)
{
    (void)arg;

    esp_err_t init_err = face_service_init();
    if (init_err != ESP_OK) {
        ESP_LOGE(SCREEN_TAG, "Face service init failed: %s", esp_err_to_name(init_err));
    }

    int face_id = -1;
    // Enroll without tying to a specific user — just tests the module enrollment path
    esp_err_t err = hlk_tx510_device_enroll(-1, &face_id);

    char msg[128];
    if (err != ESP_OK) {
        snprintf(msg, sizeof(msg), "Enroll failed: %s", esp_err_to_name(err));
        ESP_LOGE(SCREEN_TAG, "%s", msg);
    } else {
        snprintf(msg, sizeof(msg), "Enrolled! face_id=%d", face_id);
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

static void face_enroll_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Enroll button clicked");
        if (s_scan_in_progress) {
            ESP_LOGW(SCREEN_TAG, "Operation already in progress");
            return;
        }
        s_scan_in_progress = true;

        if (lvgl_port_lock(1000)) {
            if (s_status_label && lv_obj_is_valid(s_status_label)) {
                lv_label_set_text(s_status_label, "Enrolling face...");
            }
            if (s_enroll_button && lv_obj_is_valid(s_enroll_button)) {
                lv_obj_add_state(s_enroll_button, LV_STATE_DISABLED);
            }
            if (s_start_button && lv_obj_is_valid(s_start_button)) {
                lv_obj_add_state(s_start_button, LV_STATE_DISABLED);
            }
            lvgl_port_unlock();
        }

        BaseType_t created = xTaskCreate(face_enroll_task, "face_enroll", 4096,
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

void ui_screen_facescan_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static bool style_inited = false;
    if (!style_inited) {
        style_inited = true;
    }

    ui_screen_facescan = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_screen_facescan, lv_color_hex(0x041d3a), 0);
    lv_obj_set_style_text_color(ui_screen_facescan, lv_color_hex3(0xfff), 0);

#if CONFIG_LOCKBOX_FEATURE_R503 && CONFIG_LOCKBOX_FEATURE_HLK_TX510
    lv_obj_t *back_btn = lv_button_create(ui_screen_facescan);
    lv_obj_set_size(back_btn, 90, 40);
    lv_obj_set_align(back_btn, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(back_btn, 10, 10);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x444444), 0);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "< Back");
    lv_obj_set_style_text_color(back_label, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb_face, LV_EVENT_CLICKED, NULL);
#endif

    lv_obj_t *title = lv_label_create(ui_screen_facescan);
    lv_label_set_text(title, "Face Scan");
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title, 40);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);

    lv_obj_t *instr_1 = lv_label_create(ui_screen_facescan);
    lv_label_set_text(instr_1, "1.   Position your face in front of camera");
    lv_obj_set_style_text_font(instr_1, &lv_font_montserrat_26, 0);
    lv_obj_set_align(instr_1, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(instr_1, 50);
    lv_obj_set_y(instr_1, 100);

    lv_obj_t *instr_2 = lv_label_create(ui_screen_facescan);
    lv_label_set_text(instr_2, "2.   Ensure face is well lit");
    lv_obj_set_style_text_font(instr_2, &lv_font_montserrat_26, 0);
    lv_obj_set_align(instr_2, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(instr_2, 50);
    lv_obj_set_y(instr_2, 150);

    lv_obj_t *instr_3 = lv_label_create(ui_screen_facescan);
    lv_label_set_text(instr_3, "3.   Press Begin Scan");
    lv_obj_set_style_text_font(instr_3, &lv_font_montserrat_26, 0);
    lv_obj_set_align(instr_3, LV_ALIGN_TOP_LEFT);
    lv_obj_set_x(instr_3, 50);
    lv_obj_set_y(instr_3, 200);

    s_start_button = lv_button_create(ui_screen_facescan);
    lv_obj_set_align(s_start_button, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(s_start_button, lv_color_hex(0x19abe0), 0);
    lv_obj_set_pos(s_start_button, -100, 150);
    lv_obj_t *btn_label = lv_label_create(s_start_button);
    lv_label_set_text(btn_label, "Begin Scan");
    lv_obj_set_style_text_color(btn_label, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_26, 0);
    lv_obj_add_event_cb(s_start_button, face_btn_event_cb, LV_EVENT_CLICKED, NULL);

    s_enroll_button = lv_button_create(ui_screen_facescan);
    lv_obj_set_align(s_enroll_button, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(s_enroll_button, lv_color_hex(0xe07019), 0);
    lv_obj_set_pos(s_enroll_button, 100, 150);
    lv_obj_t *enroll_label = lv_label_create(s_enroll_button);
    lv_label_set_text(enroll_label, "Enroll Face");
    lv_obj_set_style_text_color(enroll_label, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_text_font(enroll_label, &lv_font_montserrat_26, 0);
    lv_obj_add_event_cb(s_enroll_button, face_enroll_btn_event_cb, LV_EVENT_CLICKED, NULL);

    s_status_label = lv_label_create(ui_screen_facescan);
    lv_label_set_text(s_status_label, "Ready to scan");
    lv_obj_set_style_text_font(s_status_label, &lv_font_montserrat_24, 0);
    lv_obj_set_align(s_status_label, LV_ALIGN_CENTER);
    lv_obj_set_y(s_status_label, 230);

    ESP_LOGI(SCREEN_TAG, "Face scan screen created");

    LV_TRACE_OBJ_CREATE("finished");
}
