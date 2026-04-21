/**
 * @file ui_screen_change_pin.c
 * @brief Two-step PIN change flow: verify current PIN, then set new PIN.
 *        In "forced" mode (first-time setup), skips current PIN verification.
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_screen_change_pin.h"
#include "../ui.h"
#include "esp_log.h"
#include "services/auth_service.h"
#include "services/session_service.h"
#include "user_store.h"
#include "lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

/*********************
 *      DEFINES
 *********************/
#define PIN_MAX_LENGTH 4
#define CHANGE_PIN_STATE_CURRENT 0
#define CHANGE_PIN_STATE_NEW     1
#define CHANGE_PIN_STATE_FORCED  2

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    int  userid;
    char pin[PIN_MAX_LENGTH + 1];
} change_pin_args_t;

/***********************
 *  STATIC VARIABLES
 **********************/
static char     pin_buffer[PIN_MAX_LENGTH + 1] = {0};
static uint8_t  pin_length = 0;
static lv_obj_t *pin_display_label   = NULL;
static lv_obj_t *show_pin_checkbox   = NULL;
static bool      show_pin            = false;
static int       s_change_userid     = -1;
static int       s_change_state      = CHANGE_PIN_STATE_CURRENT;
static char      s_current_pin_verified[PIN_MAX_LENGTH + 1] = {0};
static lv_obj_t *s_title_label       = NULL;
static lv_obj_t *s_submit_button_ptr = NULL;

/***********************
 *  STATIC PROTOTYPES
 **********************/
static void update_pin_display(void);
static void button_matrix_event_cb(lv_event_t *event);
static void submit_btn_event_cb(lv_event_t *event);
static void show_pin_checkbox_event_cb(lv_event_t *event);
static void screen_loaded_event_cb(lv_event_t *event);
static const char *SCREEN_TAG = "UI_SCREEN_CHANGE_PIN";

static void toast_timer_cb(lv_timer_t *timer);
static void navigate_to_home_cb(lv_timer_t *timer);
static void create_toast(const char *text, int timeout_ms);
static void verify_current_pin_task(void *arg);
static void save_new_pin_task(void *arg);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ui_screen_change_pin_set_context(int userid, bool forced)
{
    s_change_userid = userid;
    s_change_state  = forced ? CHANGE_PIN_STATE_FORCED : CHANGE_PIN_STATE_CURRENT;
    ESP_LOGI(SCREEN_TAG, "Context set: userid=%d, forced=%s",
             userid, forced ? "true" : "false");
}

static void update_pin_display(void)
{
    if (pin_display_label == NULL) {
        return;
    }

    char display_text[PIN_MAX_LENGTH + 1];

    if (pin_length == 0) {
        strcpy(display_text, "----");
    } else {
        for (int i = 0; i < PIN_MAX_LENGTH; i++) {
            if (i < pin_length) {
                display_text[i] = show_pin ? pin_buffer[i] : '*';
            } else {
                display_text[i] = '-';
            }
        }
        display_text[PIN_MAX_LENGTH] = '\0';
    }

    lv_label_set_text(pin_display_label, display_text);
}

static void button_matrix_event_cb(lv_event_t *event)
{
    lv_obj_t *btn_matrix = lv_event_get_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_VALUE_CHANGED) {
        uint32_t btn_id = lv_buttonmatrix_get_selected_button(btn_matrix);
        const char *btn_text = lv_buttonmatrix_get_button_text(btn_matrix, btn_id);

        if (btn_text[0] >= '0' && btn_text[0] <= '9' && btn_text[1] == '\0') {
            if (pin_length < PIN_MAX_LENGTH) {
                pin_buffer[pin_length] = btn_text[0];
                pin_length++;
                pin_buffer[pin_length] = '\0';
                update_pin_display();
            }
        } else if (strcmp(btn_text, "Clear") == 0) {
            memset(pin_buffer, 0, sizeof(pin_buffer));
            pin_length = 0;
            update_pin_display();
        } else if (strcmp(btn_text, "Backspace") == 0) {
            if (pin_length > 0) {
                pin_length--;
                pin_buffer[pin_length] = '\0';
                update_pin_display();
            }
        }
    }
}

static void submit_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;

    if (pin_length < PIN_MAX_LENGTH) {
        create_toast("Please enter a 4-digit PIN", 2000);
        return;
    }

    change_pin_args_t *args = malloc(sizeof(change_pin_args_t));
    if (!args) {
        create_toast("Out of memory", 2000);
        return;
    }
    args->userid = s_change_userid;
    memcpy(args->pin, pin_buffer, sizeof(args->pin));

    memset(pin_buffer, 0, sizeof(pin_buffer));
    pin_length = 0;
    update_pin_display();

    lv_obj_t *btn = lv_event_get_target(event);
    s_submit_button_ptr = btn;
    lv_obj_add_state(btn, LV_STATE_DISABLED);

    BaseType_t created;
    if (s_change_state == CHANGE_PIN_STATE_CURRENT) {
        created = xTaskCreate(verify_current_pin_task, "verify_pin", 4096,
                              args, tskIDLE_PRIORITY + 2, NULL);
    } else {
        // CHANGE_PIN_STATE_NEW or CHANGE_PIN_STATE_FORCED
        created = xTaskCreate(save_new_pin_task, "save_pin", 4096,
                              args, tskIDLE_PRIORITY + 2, NULL);
    }

    if (created != pdPASS) {
        free(args);
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
        create_toast("Task failed to start", 2000);
    }
}

static void show_pin_checkbox_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_VALUE_CHANGED) {
        show_pin = lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED);
        update_pin_display();
    }
}

static void screen_loaded_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_SCREEN_LOADED) return;

    memset(pin_buffer, 0, sizeof(pin_buffer));
    pin_length = 0;
    show_pin   = false;

    if (show_pin_checkbox != NULL) {
        lv_obj_clear_state(show_pin_checkbox, LV_STATE_CHECKED);
    }
    update_pin_display();

    // Resolve userid from session if not explicitly set
    if (s_change_userid == -1 && session_service_is_authenticated()) {
        s_change_userid = session_service_get_userid();
    }

    // Update title based on state
    if (s_title_label && lv_obj_is_valid(s_title_label)) {
        if (s_change_state == CHANGE_PIN_STATE_FORCED) {
            lv_label_set_text(s_title_label, "Set New PIN");
        } else if (s_change_state == CHANGE_PIN_STATE_NEW) {
            lv_label_set_text(s_title_label, "Enter New PIN");
        } else {
            lv_label_set_text(s_title_label, "Enter Current PIN");
        }
    }

    ESP_LOGI(SCREEN_TAG, "Screen loaded - state=%d, userid=%d",
             s_change_state, s_change_userid);
}

void ui_screen_change_pin_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");
    ESP_LOGI(SCREEN_TAG, "Creating change PIN screen");

    memset(pin_buffer, 0, sizeof(pin_buffer));
    pin_length = 0;
    show_pin   = false;

    ui_screen_change_pin = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_screen_change_pin, lv_color_hex(0x041d3a), 0);
    lv_obj_set_style_text_color(ui_screen_change_pin, lv_color_hex3(0xfff), 0);

    lv_obj_add_event_cb(ui_screen_change_pin, screen_loaded_event_cb,
                        LV_EVENT_SCREEN_LOADED, NULL);

    lv_obj_t *change_pin_title = lv_label_create(ui_screen_change_pin);
    lv_label_set_text(change_pin_title, "Enter Current PIN");
    lv_obj_set_style_text_font(change_pin_title, &lv_font_montserrat_28, 0);
    lv_obj_set_align(change_pin_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(change_pin_title, 10);
    s_title_label = change_pin_title;

    pin_display_label = lv_label_create(ui_screen_change_pin);
    lv_obj_set_align(pin_display_label, LV_ALIGN_CENTER);
    lv_obj_set_style_width(pin_display_label, 200, 0);
    lv_obj_set_style_height(pin_display_label, 50, 0);
    lv_obj_set_y(pin_display_label, -140);
    lv_obj_set_style_outline_width(pin_display_label, 5, 0);
    lv_obj_set_style_outline_color(pin_display_label, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_text_color(pin_display_label, lv_color_hex3(0x111), 0);
    lv_obj_set_style_text_align(pin_display_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(pin_display_label, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_bg_opa(pin_display_label, 255, 0);
    lv_obj_set_style_pad_top(pin_display_label, 12, 0);
    lv_obj_set_style_text_opa(pin_display_label, 255, 0);
    lv_obj_set_style_text_font(pin_display_label, &lv_font_montserrat_30, 0);
    lv_label_set_text(pin_display_label, "----");

    show_pin_checkbox = lv_checkbox_create(ui_screen_change_pin);
    lv_checkbox_set_text(show_pin_checkbox, "Show PIN");
    lv_obj_set_align(show_pin_checkbox, LV_ALIGN_CENTER);
    lv_obj_set_width(show_pin_checkbox, 120);
    lv_obj_set_height(show_pin_checkbox, 40);
    lv_obj_set_x(show_pin_checkbox, 180);
    lv_obj_set_y(show_pin_checkbox, -140);
    lv_obj_set_style_text_font(show_pin_checkbox, &lv_font_montserrat_16, 0);
    lv_obj_clear_state(show_pin_checkbox, LV_STATE_CHECKED);
    lv_obj_add_event_cb(show_pin_checkbox, show_pin_checkbox_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *button_matrix_pin = lv_buttonmatrix_create(ui_screen_change_pin);
    lv_obj_set_align(button_matrix_pin, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_font(button_matrix_pin, &lv_font_montserrat_26, 0);
    lv_obj_set_height(button_matrix_pin, 240);
    lv_obj_set_width(button_matrix_pin, 530);
    lv_obj_set_style_text_color(button_matrix_pin, lv_color_hex(0x1a1919), 0);
    lv_obj_set_x(button_matrix_pin, 140);
    lv_obj_set_y(button_matrix_pin, 20);
    static const char *btn_map[] = {"1","2","3","\n","4","5","6","\n",
                                    "7","8","9","\n","Clear","0","Backspace",NULL};
    lv_buttonmatrix_set_map(button_matrix_pin, btn_map);
    lv_obj_add_event_cb(button_matrix_pin, button_matrix_event_cb,
                        LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *submit_button = lv_button_create(ui_screen_change_pin);
    lv_obj_set_align(submit_button, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(submit_button, -30);
    lv_obj_set_style_bg_color(submit_button, lv_color_hex(0xe19419), 0);
    s_submit_button_ptr = submit_button;

    lv_obj_t *lv_label_0 = lv_label_create(submit_button);
    lv_label_set_text(lv_label_0, "SUBMIT");
    lv_obj_set_style_text_color(lv_label_0, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_0, &lv_font_montserrat_26, 0);

    lv_obj_add_event_cb(submit_button, submit_btn_event_cb, LV_EVENT_CLICKED, NULL);

    LV_TRACE_OBJ_CREATE("finished");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void toast_timer_cb(lv_timer_t *timer) {
    lv_obj_t *toast = lv_timer_get_user_data(timer);
    lv_obj_del(toast);
}

static void navigate_to_home_cb(lv_timer_t *timer) {
    (void)timer;
    lv_scr_load_anim(ui_screen_home, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
}

static void create_toast(const char *text, int timeout_ms) {
    lv_obj_t *toast = lv_obj_create(lv_layer_top());
    lv_obj_set_size(toast, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(toast, LV_ALIGN_TOP_LEFT, 0, 20);
    lv_obj_set_x(toast, 30);
    lv_obj_set_style_bg_color(toast, lv_color_hex(0xe19419), LV_PART_MAIN);
    lv_obj_set_style_text_font(toast, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(toast, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(toast, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(toast, 15, LV_PART_MAIN);
    lv_obj_add_flag(toast, LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t *label = lv_label_create(toast);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(label);
    lv_timer_t *timer = lv_timer_create(toast_timer_cb, timeout_ms, toast);
    lv_timer_set_repeat_count(timer, 1);
}

static void verify_current_pin_task(void *arg)
{
    change_pin_args_t *args = (change_pin_args_t *)arg;
    bool match = false;
    auth_service_verify_pin(args->userid, args->pin, &match);
    if (match) {
        memcpy(s_current_pin_verified, args->pin, sizeof(s_current_pin_verified));
    }
    free(args);

    if (lvgl_port_lock(2000)) {
        if (s_submit_button_ptr && lv_obj_is_valid(s_submit_button_ptr)) {
            lv_obj_clear_state(s_submit_button_ptr, LV_STATE_DISABLED);
        }
        if (match) {
            s_change_state = CHANGE_PIN_STATE_NEW;
            if (s_title_label && lv_obj_is_valid(s_title_label)) {
                lv_label_set_text(s_title_label, "Enter New PIN");
            }
            memset(pin_buffer, 0, sizeof(pin_buffer));
            pin_length = 0;
            update_pin_display();
        } else {
            create_toast("Incorrect current PIN.", 2500);
        }
        lvgl_port_unlock();
    }
    vTaskDelete(NULL);
}

static void save_new_pin_task(void *arg)
{
    change_pin_args_t *args = (change_pin_args_t *)arg;

    user_list_t list = {0};
    esp_err_t err = user_store_load(&list);
    bool saved = false;
    if (err == ESP_OK) {
        for (size_t i = 0; i < list.count; i++) {
            if (list.items[i].userid == args->userid) {
                strncpy(list.items[i].pin, args->pin, USER_STORE_MAX_PIN_LEN);
                list.items[i].pin[USER_STORE_MAX_PIN_LEN] = '\0';
                err   = user_store_save(&list);
                saved = (err == ESP_OK);
                break;
            }
        }
        user_store_free(&list);
    }
    free(args);

    if (lvgl_port_lock(2000)) {
        if (s_submit_button_ptr && lv_obj_is_valid(s_submit_button_ptr)) {
            lv_obj_clear_state(s_submit_button_ptr, LV_STATE_DISABLED);
        }
        if (saved) {
            create_toast("PIN changed successfully!", 2000);
            lv_timer_create(navigate_to_home_cb, 2500, NULL);
        } else {
            create_toast("Failed to save PIN. Try again.", 2500);
        }
        lvgl_port_unlock();
    }
    vTaskDelete(NULL);
}
