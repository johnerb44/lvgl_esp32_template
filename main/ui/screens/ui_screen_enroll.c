/**
 * @file ui_screen_enroll.c
 * @brief Biometric enrollment screen – fingerprint (required) + face (optional)
 */

#include "ui_screen_enroll.h"
#include "../ui.h"
#include "ui_screen_change_pin.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl_port.h"
#include "services/fingerprint_service.h"
#include "services/face_service.h"
#include "services/session_service.h"
#include "user_store.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

static const char *SCREEN_TAG = "UI_SCREEN_ENROLL";

static lv_obj_t *s_username_label   = NULL;
static lv_obj_t *s_fp_status_label  = NULL;
static lv_obj_t *s_face_status_label= NULL;
static lv_obj_t *s_fp_enroll_btn    = NULL;
static lv_obj_t *s_face_enroll_btn  = NULL;
static lv_obj_t *s_done_btn         = NULL;
static lv_obj_t *s_status_label     = NULL;

static int  s_enroll_userid          = -1;
static bool s_from_first_time_setup  = false;
static bool s_fp_enrolled            = false;
static bool s_face_enrolled          = false;
static bool s_fp_task_running        = false;
static bool s_face_task_running      = false;

// ── context setter ────────────────────────────────────────────────────────────

void ui_screen_enroll_set_context(int userid, bool from_first_time_setup)
{
    s_enroll_userid         = userid;
    s_from_first_time_setup = from_first_time_setup;
    // Enrollment status and label updates are handled in screen_loaded_event_cb
    // (safe to call from inside LVGL lock since no SD access here)
}

// ── background task: load user info and refresh labels ───────────────────────

static void enroll_screen_refresh_task(void *arg)
{
    (void)arg;
    user_list_t list = {0};
    char username[USER_STORE_MAX_USERNAME_LEN + 1] = {0};
    bool found       = false;
    bool fp_enrolled = false;
    bool face_enrolled = false;

    if (user_store_load(&list) == ESP_OK) {
        for (size_t i = 0; i < list.count; i++) {
            if (list.items[i].userid == s_enroll_userid) {
                strncpy(username, list.items[i].username, USER_STORE_MAX_USERNAME_LEN);
                username[USER_STORE_MAX_USERNAME_LEN] = '\0';
                fp_enrolled   = (list.items[i].fingerid >= 0);
                face_enrolled = (list.items[i].faceid >= 0);
                found = true;
                break;
            }
        }
        user_store_free(&list);
    }

    if (!found) {
        strncpy(username, "Unknown User", sizeof(username) - 1);
    }
    s_fp_enrolled   = fp_enrolled;
    s_face_enrolled = face_enrolled;

    if (lvgl_port_lock(2000)) {
        if (s_username_label && lv_obj_is_valid(s_username_label)) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Enrolling: %s", username);
            lv_label_set_text(s_username_label, buf);
        }
        if (s_fp_status_label && lv_obj_is_valid(s_fp_status_label)) {
            lv_label_set_text(s_fp_status_label,
                fp_enrolled ? "Fingerprint: Enrolled " LV_SYMBOL_OK
                            : "Fingerprint: Not Enrolled (required)");
        }
        if (s_face_status_label && lv_obj_is_valid(s_face_status_label)) {
            lv_label_set_text(s_face_status_label,
                face_enrolled ? "Face: Enrolled " LV_SYMBOL_OK
                              : "Face: Not Enrolled (optional)");
        }
        if (s_done_btn && lv_obj_is_valid(s_done_btn)) {
            if (fp_enrolled) {
                lv_obj_clear_state(s_done_btn, LV_STATE_DISABLED);
            } else {
                lv_obj_add_state(s_done_btn, LV_STATE_DISABLED);
            }
        }
        lvgl_port_unlock();
    }
    vTaskDelete(NULL);
}

// ── screen loaded event ───────────────────────────────────────────────────────

static void enroll_screen_loaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_LOADED) return;
    ESP_LOGI(SCREEN_TAG, "Screen loaded for userid=%d", s_enroll_userid);
    xTaskCreate(enroll_screen_refresh_task, "enroll_refresh", 4096,
                NULL, tskIDLE_PRIORITY + 2, NULL);
}

// ── FP enroll background task ─────────────────────────────────────────────────

static void fp_enroll_task(void *arg)
{
    (void)arg;
    r503_status_t status     = R503_STATUS_OK;
    int           template_id = -1;
    esp_err_t     err = fingerprint_service_enroll_user(s_enroll_userid, &template_id, &status);

    if (lvgl_port_lock(1000)) {
        if (err == ESP_OK) {
            s_fp_enrolled = true;
            if (s_fp_status_label && lv_obj_is_valid(s_fp_status_label)) {
                lv_label_set_text(s_fp_status_label,
                                  "Fingerprint: Enrolled " LV_SYMBOL_OK);
            }
            if (s_done_btn && lv_obj_is_valid(s_done_btn)) {
                lv_obj_clear_state(s_done_btn, LV_STATE_DISABLED);
            }
            if (s_status_label && lv_obj_is_valid(s_status_label)) {
                lv_label_set_text(s_status_label, "Fingerprint enrolled successfully!");
            }
        } else {
            char msg[96];
            snprintf(msg, sizeof(msg), "FP enroll failed: %s",
                     fingerprint_service_status_to_string(status));
            if (s_status_label && lv_obj_is_valid(s_status_label)) {
                lv_label_set_text(s_status_label, msg);
            }
        }
        if (s_fp_enroll_btn && lv_obj_is_valid(s_fp_enroll_btn)) {
            lv_obj_clear_state(s_fp_enroll_btn, LV_STATE_DISABLED);
        }
        s_fp_task_running = false;
        lvgl_port_unlock();
    }
    vTaskDelete(NULL);
}

// ── Face enroll background task ───────────────────────────────────────────────

static void face_enroll_task(void *arg)
{
    (void)arg;
    int       face_id = -1;
    esp_err_t err = face_service_enroll_user(s_enroll_userid, &face_id);

    if (lvgl_port_lock(1000)) {
        if (err == ESP_OK) {
            s_face_enrolled = true;
            if (s_face_status_label && lv_obj_is_valid(s_face_status_label)) {
                lv_label_set_text(s_face_status_label,
                                  "Face: Enrolled " LV_SYMBOL_OK);
            }
            if (s_status_label && lv_obj_is_valid(s_status_label)) {
                lv_label_set_text(s_status_label, "Face enrolled successfully!");
            }
        } else {
            char msg[96];
            snprintf(msg, sizeof(msg), "Face enroll failed: %s", esp_err_to_name(err));
            if (s_status_label && lv_obj_is_valid(s_status_label)) {
                lv_label_set_text(s_status_label, msg);
            }
        }
        if (s_face_enroll_btn && lv_obj_is_valid(s_face_enroll_btn)) {
            lv_obj_clear_state(s_face_enroll_btn, LV_STATE_DISABLED);
        }
        s_face_task_running = false;
        lvgl_port_unlock();
    }
    vTaskDelete(NULL);
}

// ── button event callbacks ────────────────────────────────────────────────────

static void fp_enroll_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if (s_fp_task_running) return;
    s_fp_task_running = true;
    lv_obj_t *btn = lv_event_get_target(event);
    lv_obj_add_state(btn, LV_STATE_DISABLED);
    if (s_status_label && lv_obj_is_valid(s_status_label)) {
        lv_label_set_text(s_status_label, "Scanning fingerprint...");
    }
    BaseType_t created = xTaskCreate(fp_enroll_task, "fp_enroll", 4096,
                                     NULL, tskIDLE_PRIORITY + 2, NULL);
    if (created != pdPASS) {
        s_fp_task_running = false;
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        if (s_status_label && lv_obj_is_valid(s_status_label)) {
            lv_label_set_text(s_status_label, "Failed to start enroll task");
        }
    }
}

static void face_enroll_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if (s_face_task_running) return;
    s_face_task_running = true;
    lv_obj_t *btn = lv_event_get_target(event);
    lv_obj_add_state(btn, LV_STATE_DISABLED);
    if (s_status_label && lv_obj_is_valid(s_status_label)) {
        lv_label_set_text(s_status_label, "Scanning face...");
    }
    BaseType_t created = xTaskCreate(face_enroll_task, "face_enroll", 4096,
                                     NULL, tskIDLE_PRIORITY + 2, NULL);
    if (created != pdPASS) {
        s_face_task_running = false;
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        if (s_status_label && lv_obj_is_valid(s_status_label)) {
            lv_label_set_text(s_status_label, "Failed to start face enroll task");
        }
    }
}

static void done_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if (!s_fp_enrolled) return;  // guard: button should be disabled, but safety check

    if (s_from_first_time_setup) {
        ui_screen_change_pin_set_context(s_enroll_userid, true);
        lv_scr_load_anim(ui_screen_change_pin, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
    } else {
        lv_scr_load_anim(ui_screen_home, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
    }
}

static void back_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
    if (session_service_is_authenticated()) {
        lv_scr_load_anim(ui_screen_home, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 500, 0, false);
    } else {
        lv_scr_load_anim(ui_screen_main, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 500, 0, false);
    }
}

// ── screen create ─────────────────────────────────────────────────────────────

void ui_screen_enroll_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    ui_screen_enroll = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_screen_enroll, lv_color_hex(0x041d3a), 0);
    lv_obj_set_style_text_color(ui_screen_enroll, lv_color_hex(0xffffff), 0);

    lv_obj_add_event_cb(ui_screen_enroll, enroll_screen_loaded_cb,
                        LV_EVENT_SCREEN_LOADED, NULL);

    // Title
    lv_obj_t *title = lv_label_create(ui_screen_enroll);
    lv_label_set_text(title, "Biometric Enrollment");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title, 10);

    // Username label
    s_username_label = lv_label_create(ui_screen_enroll);
    lv_label_set_text(s_username_label, "Enrolling: ...");
    lv_obj_set_style_text_color(s_username_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_text_font(s_username_label, &lv_font_montserrat_20, 0);
    lv_obj_set_align(s_username_label, LV_ALIGN_CENTER);
    lv_obj_set_y(s_username_label, -170);

    // Fingerprint status label — left edge 15px right of Enroll Fingerprint button right edge
    // Button: center x=0, width=220 → right edge at screen_center+110 = px 510; label starts at px 525
    s_fp_status_label = lv_label_create(ui_screen_enroll);
    lv_label_set_text(s_fp_status_label, "Fingerprint: Not Enrolled (required)");
    lv_obj_set_style_text_color(s_fp_status_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_align(s_fp_status_label, LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(s_fp_status_label, 525, -40);

    // Fingerprint enroll button (centered)
    s_fp_enroll_btn = lv_button_create(ui_screen_enroll);
    lv_obj_set_align(s_fp_enroll_btn, LV_ALIGN_CENTER);
    lv_obj_set_x(s_fp_enroll_btn, 0);
    lv_obj_set_y(s_fp_enroll_btn, -40);
    lv_obj_set_width(s_fp_enroll_btn, 220);
    lv_obj_set_style_bg_color(s_fp_enroll_btn, lv_color_hex(0x21a019), 0);
    lv_obj_t *fp_label = lv_label_create(s_fp_enroll_btn);
    lv_label_set_text(fp_label, "Enroll Fingerprint");
    lv_obj_set_style_text_color(fp_label, lv_color_hex(0xffffff), 0);
    lv_obj_center(fp_label);
    lv_obj_add_event_cb(s_fp_enroll_btn, fp_enroll_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Face status label — left edge 15px right of Enroll Face button right edge
    // Button: center x=0, width=220 → right edge at screen_center+110 = px 510; label starts at px 525
    s_face_status_label = lv_label_create(ui_screen_enroll);
    lv_label_set_text(s_face_status_label, "Face: Not Enrolled (optional)");
    lv_obj_set_style_text_color(s_face_status_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_align(s_face_status_label, LV_ALIGN_LEFT_MID);
    lv_obj_set_pos(s_face_status_label, 525, 90);

    // Face enroll button (centered)
    s_face_enroll_btn = lv_button_create(ui_screen_enroll);
    lv_obj_set_align(s_face_enroll_btn, LV_ALIGN_CENTER);
    lv_obj_set_x(s_face_enroll_btn, 0);
    lv_obj_set_y(s_face_enroll_btn, 90);
    lv_obj_set_width(s_face_enroll_btn, 220);
    lv_obj_set_style_bg_color(s_face_enroll_btn, lv_color_hex(0x1975e0), 0);
    lv_obj_t *face_label = lv_label_create(s_face_enroll_btn);
    lv_label_set_text(face_label, "Enroll Face (Optional)");
    lv_obj_set_style_text_color(face_label, lv_color_hex(0xffffff), 0);
    lv_obj_center(face_label);
    lv_obj_add_event_cb(s_face_enroll_btn, face_enroll_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // General status label
    s_status_label = lv_label_create(ui_screen_enroll);
    lv_label_set_text(s_status_label, "");
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_align(s_status_label, LV_ALIGN_CENTER);
    lv_obj_set_y(s_status_label, 150);

    // Done button (starts disabled)
    s_done_btn = lv_button_create(ui_screen_enroll);
    lv_obj_set_align(s_done_btn, LV_ALIGN_CENTER);
    lv_obj_set_y(s_done_btn, 210);
    lv_obj_set_width(s_done_btn, 200);
    lv_obj_set_style_bg_color(s_done_btn, lv_color_hex(0xe19419), 0);
    lv_obj_add_state(s_done_btn, LV_STATE_DISABLED);
    lv_obj_t *done_label = lv_label_create(s_done_btn);
    lv_label_set_text(done_label, "Done");
    lv_obj_set_style_text_color(done_label, lv_color_hex(0xffffff), 0);
    lv_obj_center(done_label);
    lv_obj_add_event_cb(s_done_btn, done_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Back button (bottom-left)
    lv_obj_t *back_btn = lv_button_create(ui_screen_enroll);
    lv_obj_set_align(back_btn, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_pos(back_btn, 10, -10);
    lv_obj_set_width(back_btn, 100);
    lv_obj_set_height(back_btn, 40);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x555555), 0);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "< Back");
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xffffff), 0);
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    LV_TRACE_OBJ_CREATE("finished");
    ESP_LOGI(SCREEN_TAG, "Enroll screen created");
}
