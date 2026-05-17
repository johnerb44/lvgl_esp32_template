/**
 * @file ui_screen_fp_manage.c
 * @brief FP Template Manager screen — lists sensor templates with user linkage.
 *        Orphan templates (not linked to any user) can be safely deleted here.
 */

#include "ui_screen_fp_manage.h"
#include "../ui.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl_port.h"
#include "services/fingerprint_service.h"
#include "user_store.h"
#include "devices/r503_device.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *SCREEN_TAG = "UI_SCREEN_FP_MANAGE";

// Maximum sensor templates to scan (page 0 covers IDs 0–255; R503 stores up to 200)
#define FP_MANAGE_MAX_TEMPLATES 256

// ui_screen_fp_manage is owned by ui.c; this file only writes to it via ui_screen_fp_manage_create()
static lv_obj_t *s_list_container  = NULL;
static lv_obj_t *s_status_label    = NULL;
static bool      s_task_running    = false;

// ── types ─────────────────────────────────────────────────────────────────────

typedef struct {
    int  id;
    bool linked;
    char username[USER_STORE_MAX_USERNAME_LEN + 1];
} tmpl_info_t;

// ── forward declarations ──────────────────────────────────────────────────────

static void fp_manage_load_task(void *pvParam);

// ── helpers ───────────────────────────────────────────────────────────────────

static void launch_load_task(void)
{
    if (s_task_running) {
        return;
    }
    s_task_running = true;
    BaseType_t created = xTaskCreate(fp_manage_load_task, "fp_mgr_load", 6144,
                                     NULL, tskIDLE_PRIORITY + 2, NULL);
    if (created != pdPASS) {
        s_task_running = false;
        ESP_LOGE(SCREEN_TAG, "Failed to create load task");
    }
}

// ── delete button callback ────────────────────────────────────────────────────

static void delete_btn_event_cb(lv_event_t *e);

typedef struct {
    int template_id;
} delete_task_param_t;

static void fp_manage_delete_task(void *pvParam)
{
    delete_task_param_t *p = (delete_task_param_t *)pvParam;
    int template_id = p->template_id;
    free(p);

    ESP_LOGI(SCREEN_TAG, "Deleting orphan template #%d", template_id);
    r503_status_t status;
    esp_err_t err = fingerprint_service_delete_template(template_id, &status);

    const char *msg;
    lv_color_t msg_color;
    if (err == ESP_OK && status == R503_STATUS_OK) {
        ESP_LOGI(SCREEN_TAG, "Deleted template #%d", template_id);
        msg       = "Template deleted";
        msg_color = lv_color_hex(0x44cc44);
    } else {
        ESP_LOGE(SCREEN_TAG, "Delete failed for template #%d: %s", template_id,
                 fingerprint_service_status_to_string(status));
        msg       = "Delete failed";
        msg_color = lv_color_hex(0xff4444);
    }

    if (lvgl_port_lock(2000)) {
        if (s_status_label && lv_obj_is_valid(s_status_label)) {
            lv_label_set_text(s_status_label, msg);
            lv_obj_set_style_text_color(s_status_label, msg_color, 0);
        }
        if (s_list_container && lv_obj_is_valid(s_list_container)) {
            lv_obj_clean(s_list_container);
        }
        lvgl_port_unlock();
    }

    // Repopulate the list after deletion
    s_task_running = false;
    launch_load_task();
    vTaskDelete(NULL);
}

static void delete_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (s_task_running) return;

    lv_obj_t *btn = lv_event_get_target(e);
    int template_id = (int)(intptr_t)lv_obj_get_user_data(btn);

    lv_obj_add_state(btn, LV_STATE_DISABLED);

    if (s_status_label && lv_obj_is_valid(s_status_label)) {
        char msg[40];
        snprintf(msg, sizeof(msg), "Deleting template #%d...", template_id);
        lv_label_set_text(s_status_label, msg);
        lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xffffff), 0);
    }

    delete_task_param_t *p = malloc(sizeof(delete_task_param_t));
    if (p == NULL) {
        ESP_LOGE(SCREEN_TAG, "OOM for delete_task_param");
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        return;
    }
    p->template_id = template_id;

    s_task_running = true;
    BaseType_t created = xTaskCreate(fp_manage_delete_task, "fp_mgr_del", 6144,
                                     p, tskIDLE_PRIORITY + 2, NULL);
    if (created != pdPASS) {
        s_task_running = false;
        free(p);
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        ESP_LOGE(SCREEN_TAG, "Failed to create delete task");
    }
}

// ── load task — reads sensor bitmap + user store, populates list ──────────────

static void fp_manage_load_task(void *pvParam)
{
    (void)pvParam;

    // 1. Read sensor index table
    uint8_t bitmap[32] = {0};
    r503_status_t st   = R503_STATUS_SENSOR_ERROR;
    bool sensor_ok     = (r503_device_read_index_table(0, bitmap, &st) == ESP_OK &&
                          st == R503_STATUS_OK);
    if (!sensor_ok) {
        ESP_LOGE(SCREEN_TAG, "Failed to read sensor index table: %s",
                 fingerprint_service_status_to_string(st));
    }

    // 2. Load user store
    user_list_t ulist = {0};
    bool store_ok = (user_store_load(&ulist) == ESP_OK);

    // 3. Build template info list (heap-allocated)
    tmpl_info_t *templates = NULL;
    int tmpl_count = 0;

    if (sensor_ok) {
        templates = malloc(sizeof(tmpl_info_t) * FP_MANAGE_MAX_TEMPLATES);
        if (templates == NULL) {
            ESP_LOGE(SCREEN_TAG, "OOM for template info array");
            sensor_ok = false;
        }
    }

    if (sensor_ok && templates != NULL) {
        for (int i = 0; i < 256; i++) {
            int byte_idx = i / 8;
            int bit_idx  = i % 8;
            if (!((bitmap[byte_idx] >> bit_idx) & 1)) {
                continue; // empty slot
            }
            templates[tmpl_count].id     = i;
            templates[tmpl_count].linked = false;
            templates[tmpl_count].username[0] = '\0';

            if (store_ok) {
                for (size_t j = 0; j < ulist.count; j++) {
                    if (ulist.items[j].fingerid == i) {
                        templates[tmpl_count].linked = true;
                        strncpy(templates[tmpl_count].username,
                                ulist.items[j].username,
                                USER_STORE_MAX_USERNAME_LEN);
                        templates[tmpl_count].username[USER_STORE_MAX_USERNAME_LEN] = '\0';
                        break;
                    }
                }
            }
            tmpl_count++;
        }
    }

    if (store_ok) {
        user_store_free(&ulist);
    }

    // Count orphans for the status label
    int orphan_count = 0;
    for (int i = 0; i < tmpl_count; i++) {
        if (!templates[i].linked) orphan_count++;
    }

    // 4. Populate UI under LVGL lock
    if (lvgl_port_lock(2000)) {
        if (s_list_container && lv_obj_is_valid(s_list_container)) {
            lv_obj_clean(s_list_container);

            if (!sensor_ok) {
                lv_obj_t *lbl = lv_label_create(s_list_container);
                lv_label_set_text(lbl, "Error reading sensor template table");
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xff4444), 0);

            } else if (tmpl_count == 0) {
                lv_obj_t *lbl = lv_label_create(s_list_container);
                lv_label_set_text(lbl, "No templates stored on sensor");
                lv_obj_set_style_text_color(lbl, lv_color_hex(0xaaaaaa), 0);

            } else {
                for (int i = 0; i < tmpl_count; i++) {
                    bool orphan = !templates[i].linked;
                    lv_color_t row_bg = orphan ? lv_color_hex(0x3a1a1a) : lv_color_hex(0x1a3a5c);

                    lv_obj_t *row = lv_obj_create(s_list_container);
                    lv_obj_set_size(row, LV_PCT(100), 48);
                    lv_obj_set_style_bg_color(row, row_bg, 0);
                    lv_obj_set_style_border_width(row, 0, 0);
                    lv_obj_set_style_pad_all(row, 6, 0);
                    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
                    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                    lv_obj_set_scrollbar_mode(row, LV_SCROLLBAR_MODE_OFF);

                    // Template description label
                    char tmpl_text[72];
                    if (orphan) {
                        snprintf(tmpl_text, sizeof(tmpl_text),
                                 "Template #%d  —  orphan (not linked to any user)", templates[i].id);
                    } else {
                        snprintf(tmpl_text, sizeof(tmpl_text),
                                 "Template #%d  —  %s", templates[i].id, templates[i].username);
                    }
                    lv_obj_t *lbl = lv_label_create(row);
                    lv_label_set_text(lbl, tmpl_text);
                    lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
                    lv_obj_set_flex_grow(lbl, 1);

                    // Action column
                    if (orphan) {
                        lv_obj_t *del_btn = lv_button_create(row);
                        lv_obj_set_size(del_btn, 90, 34);
                        lv_obj_set_style_bg_color(del_btn, lv_color_hex(0xcc2222), 0);
                        lv_obj_set_user_data(del_btn, (void *)(intptr_t)templates[i].id);
                        lv_obj_t *del_lbl = lv_label_create(del_btn);
                        lv_label_set_text(del_lbl, "Delete");
                        lv_obj_set_style_text_color(del_lbl, lv_color_hex(0xffffff), 0);
                        lv_obj_center(del_lbl);
                        lv_obj_add_event_cb(del_btn, delete_btn_event_cb, LV_EVENT_CLICKED, NULL);
                    } else {
                        lv_obj_t *tag_lbl = lv_label_create(row);
                        lv_label_set_text(tag_lbl, LV_SYMBOL_OK " Linked");
                        lv_obj_set_style_text_color(tag_lbl, lv_color_hex(0x44cc44), 0);
                    }
                }
            }
        }

        if (s_status_label && lv_obj_is_valid(s_status_label)) {
            char status_text[72];
            if (!sensor_ok) {
                lv_label_set_text(s_status_label, "Sensor error");
                lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xff4444), 0);
            } else {
                snprintf(status_text, sizeof(status_text),
                         "%d template(s) total  |  %d orphan(s)", tmpl_count, orphan_count);
                lv_label_set_text(s_status_label, status_text);
                lv_color_t sc = orphan_count > 0 ? lv_color_hex(0xffcc44) : lv_color_hex(0x44cc44);
                lv_obj_set_style_text_color(s_status_label, sc, 0);
            }
        }

        lvgl_port_unlock();
    }

    free(templates);
    s_task_running = false;
    vTaskDelete(NULL);
}

// ── screen events ─────────────────────────────────────────────────────────────

static void fp_manage_screen_loaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_LOADED) return;

    if (s_status_label && lv_obj_is_valid(s_status_label)) {
        lv_label_set_text(s_status_label, "Scanning sensor...");
        lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xffffff), 0);
    }
    if (s_list_container && lv_obj_is_valid(s_list_container)) {
        lv_obj_clean(s_list_container);
    }
    s_task_running = false;
    launch_load_task();
}

static void back_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lv_scr_load_anim(ui_screen_enroll, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
}

static void refresh_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (s_task_running) return;

    if (s_status_label && lv_obj_is_valid(s_status_label)) {
        lv_label_set_text(s_status_label, "Scanning sensor...");
        lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xffffff), 0);
    }
    if (s_list_container && lv_obj_is_valid(s_list_container)) {
        lv_obj_clean(s_list_container);
    }
    launch_load_task();
}

// ── screen create ─────────────────────────────────────────────────────────────

void ui_screen_fp_manage_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    ui_screen_fp_manage = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_screen_fp_manage, lv_color_hex(0x041d3a), 0);
    lv_obj_set_style_text_color(ui_screen_fp_manage, lv_color_hex(0xffffff), 0);
    lv_obj_add_event_cb(ui_screen_fp_manage, fp_manage_screen_loaded_cb,
                        LV_EVENT_SCREEN_LOADED, NULL);

    // Title
    lv_obj_t *title = lv_label_create(ui_screen_fp_manage);
    lv_label_set_text(title, "FP Template Manager");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_align(title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title, 10);

    // Scrollable list container
    s_list_container = lv_obj_create(ui_screen_fp_manage);
    lv_obj_set_size(s_list_container, 760, 320);
    lv_obj_set_align(s_list_container, LV_ALIGN_TOP_MID);
    lv_obj_set_y(s_list_container, 55);
    lv_obj_set_style_bg_color(s_list_container, lv_color_hex(0x0a2a4a), 0);
    lv_obj_set_style_border_color(s_list_container, lv_color_hex(0x1e5080), 0);
    lv_obj_set_style_border_width(s_list_container, 1, 0);
    lv_obj_set_style_pad_all(s_list_container, 6, 0);
    lv_obj_set_style_pad_row(s_list_container, 4, 0);
    lv_obj_set_flex_flow(s_list_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_list_container,
                          LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Status label
    s_status_label = lv_label_create(ui_screen_fp_manage);
    lv_label_set_text(s_status_label, "");
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_align(s_status_label, LV_ALIGN_TOP_MID);
    lv_obj_set_y(s_status_label, 385);

    // Back button (bottom-left)
    lv_obj_t *back_btn = lv_button_create(ui_screen_fp_manage);
    lv_obj_set_align(back_btn, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_pos(back_btn, 10, -10);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x555555), 0);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "< Back");
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(back_lbl);
    lv_obj_add_event_cb(back_btn, back_btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Refresh button (bottom-right)
    lv_obj_t *refresh_btn = lv_button_create(ui_screen_fp_manage);
    lv_obj_set_align(refresh_btn, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_pos(refresh_btn, -10, -10);
    lv_obj_set_size(refresh_btn, 120, 40);
    lv_obj_set_style_bg_color(refresh_btn, lv_color_hex(0x1975e0), 0);
    lv_obj_t *refresh_lbl = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_lbl, LV_SYMBOL_REFRESH " Refresh");
    lv_obj_set_style_text_color(refresh_lbl, lv_color_hex(0xffffff), 0);
    lv_obj_center(refresh_lbl);
    lv_obj_add_event_cb(refresh_btn, refresh_btn_event_cb, LV_EVENT_CLICKED, NULL);

    LV_TRACE_OBJ_CREATE("finished");
    ESP_LOGI(SCREEN_TAG, "FP Manage screen created");
}
