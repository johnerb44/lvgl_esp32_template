/**
 * @file ui_screen_home.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_screen_home.h"
#include "../ui.h"
#include "esp_log.h"
#include "user_mgmt_ui.h"
#include "services/lock_service.h"
#include "services/session_service.h"
#include "ui/screens/ui_screen_change_pin.h"
#include "ui/screens/ui_screen_enroll.h"
#include "sdkconfig.h"
#if CONFIG_LOCKBOX_FEATURE_STATUS_INPUTS
#include "devices/status_inputs.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t *s_user_label = NULL;
static lv_obj_t *s_lock_label = NULL;
static lv_obj_t *s_lid_label  = NULL;
static lv_obj_t *s_battery_box = NULL;

/***********************
 *  STATIC PROTOTYPES
 **********************/
static const char *SCREEN_TAG = "UI_SCREEN_HOME";
static void admin_button_event_handler(lv_event_t * e);
static void unlock_btn_event_cb(lv_event_t *e);
static void change_pin_btn_event_cb(lv_event_t *e);
static void register_btn_event_cb(lv_event_t *e);
static void signout_btn_event_cb(lv_event_t *e);
static void home_screen_loaded_cb(lv_event_t *e);
static void toast_timer_cb(lv_timer_t *timer);
static void create_toast(const char *text, int timeout_ms);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ui_screen_home_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    static bool style_inited = false;

    if (!style_inited) {
        
        style_inited = true;
    }

    ui_screen_home = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_screen_home, lv_color_hex(0x041d3a), 0);
    lv_obj_set_style_text_color(ui_screen_home, lv_color_hex3(0xfff), 0);

    lv_obj_add_event_cb(ui_screen_home, home_screen_loaded_cb,
                        LV_EVENT_SCREEN_LOADED, NULL);

    lv_obj_t * title_label = lv_label_create(ui_screen_home);
    lv_label_set_text(title_label, "Secure Lock Box");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_28, 0);
    lv_obj_set_align(title_label, LV_ALIGN_TOP_MID);
    lv_obj_set_y(title_label, 10);

    // Welcome / current user label
    s_user_label = lv_label_create(ui_screen_home);
    lv_label_set_text(s_user_label, "");
    lv_obj_set_style_text_font(s_user_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_user_label, lv_color_hex(0xffffff), 0);
    lv_obj_set_align(s_user_label, LV_ALIGN_TOP_MID);
    lv_obj_set_y(s_user_label, 50);

    // Lock status badge (top-right)
    s_lock_label = lv_label_create(ui_screen_home);
    lv_label_set_text(s_lock_label, "");
    lv_obj_set_style_text_font(s_lock_label, &lv_font_montserrat_20, 0);
    lv_obj_set_align(s_lock_label, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_pos(s_lock_label, -10, 10);

    // Lid status indicator
    s_lid_label = lv_label_create(ui_screen_home);
    lv_label_set_text(s_lid_label, "Lid: --");
    lv_obj_set_style_text_font(s_lid_label, &lv_font_montserrat_20, 0);
    lv_obj_align(s_lid_label, LV_ALIGN_TOP_RIGHT, -10, 56);

    // Battery status indicator — label + colored rectangle
    lv_obj_t *bat_title = lv_label_create(ui_screen_home);
    lv_label_set_text(bat_title, "Battery");
    lv_obj_set_style_text_font(bat_title, &lv_font_montserrat_20, 0);
    lv_obj_align(bat_title, LV_ALIGN_TOP_RIGHT, -10, 102);

    s_battery_box = lv_obj_create(ui_screen_home);
    lv_obj_set_size(s_battery_box, 80, 22);
    lv_obj_align(s_battery_box, LV_ALIGN_TOP_RIGHT, -10, 132);
    lv_obj_set_style_bg_color(s_battery_box, lv_color_hex(0x44cc44), 0);
    lv_obj_set_style_border_color(s_battery_box, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_border_width(s_battery_box, 2, 0);
    lv_obj_set_style_radius(s_battery_box, 4, 0);
    lv_obj_clear_flag(s_battery_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    
    lv_obj_t * unlock_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(unlock_button, LV_ALIGN_CENTER);
    lv_obj_set_width(unlock_button, 420);
    lv_obj_set_y(unlock_button, -120);
    lv_obj_set_style_bg_color(unlock_button, lv_color_hex(0xe19419), 0);
    
    lv_obj_t * lv_label_0 = lv_label_create(unlock_button);
    lv_label_set_text(lv_label_0, "Unlock Secure Box");
    lv_obj_set_style_text_color(lv_label_0, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_0,  &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_align(lv_label_0, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lv_label_0);
    
    lv_obj_add_event_cb(unlock_button, unlock_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * change_pin_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(change_pin_button, LV_ALIGN_CENTER);
    lv_obj_set_width(change_pin_button, 420);
    lv_obj_set_y(change_pin_button, -50);
    lv_obj_set_style_bg_color(change_pin_button, lv_color_hex(0x197de0), 0);
    
    lv_obj_t * lv_label_1 = lv_label_create(change_pin_button);
    lv_label_set_text(lv_label_1, "Change PIN");
    lv_obj_set_style_text_color(lv_label_1, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_1,  &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_align(lv_label_1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lv_label_1);
    
    lv_obj_add_event_cb(change_pin_button, change_pin_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * register_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(register_button, LV_ALIGN_CENTER);
    lv_obj_set_width(register_button, 420);
    lv_obj_set_y(register_button, 20);
    lv_obj_set_style_bg_color(register_button, lv_color_hex(0x21a019), 0);
    
    lv_obj_t * lv_label_2 = lv_label_create(register_button);
    lv_label_set_text(lv_label_2, "Register Biometrics");
    lv_obj_set_style_text_color(lv_label_2, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_2, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_align(lv_label_2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lv_label_2);
    
    lv_obj_add_event_cb(register_button, register_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * admin_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(admin_button, LV_ALIGN_CENTER);
    lv_obj_set_width(admin_button, 420);
    lv_obj_set_y(admin_button, 90);
    lv_obj_set_style_bg_color(admin_button, lv_color_hex(0x8719e0), 0);
    
    lv_obj_t * lv_label_3 = lv_label_create(admin_button);
    lv_label_set_text(lv_label_3, "Admin");
    lv_obj_set_style_text_color(lv_label_3, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_3, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_align(lv_label_3, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lv_label_3);
    
    lv_obj_add_event_cb(admin_button, admin_button_event_handler, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * signout_button = lv_button_create(ui_screen_home);
    lv_obj_set_align(signout_button, LV_ALIGN_CENTER);
    lv_obj_set_width(signout_button, 420);
    lv_obj_set_y(signout_button, 160);
    lv_obj_set_style_bg_color(signout_button, lv_color_hex(0xd9e019), 0);
    
    lv_obj_t * lv_label_4 = lv_label_create(signout_button);
    lv_label_set_text(lv_label_4, "Sign Out / Lock Secure Box");
    lv_obj_set_style_text_color(lv_label_4, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_4, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_align(lv_label_4, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lv_label_4);
    
    lv_obj_add_event_cb(signout_button, signout_btn_event_cb, LV_EVENT_CLICKED, NULL);

    ESP_LOGI(SCREEN_TAG, "Home screen created");

    LV_TRACE_OBJ_CREATE("finished");

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void toast_timer_cb(lv_timer_t *timer) {
    lv_obj_t *toast = lv_timer_get_user_data(timer);
    lv_obj_del(toast);
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

static void home_screen_loaded_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_SCREEN_LOADED) return;

    if (s_user_label && lv_obj_is_valid(s_user_label)) {
        user_t u = {0};
        if (session_service_get_user(&u) == ESP_OK) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Welcome, %s", u.username);
            lv_label_set_text(s_user_label, buf);
        } else {
            lv_label_set_text(s_user_label, "");
        }
    }

    if (s_lock_label && lv_obj_is_valid(s_lock_label)) {
        lock_state_t state = {0};
        lock_service_get_state(&state);
        if (state.is_locked) {
            lv_label_set_text(s_lock_label, "Box: Locked");
            lv_obj_set_style_text_color(s_lock_label, lv_color_hex(0xff4444), 0);
        } else {
            lv_label_set_text(s_lock_label, "Box: Unlocked");
            lv_obj_set_style_text_color(s_lock_label, lv_color_hex(0x44ff44), 0);
        }
    }

    // Lid status
    if (s_lid_label && lv_obj_is_valid(s_lid_label)) {
#if CONFIG_LOCKBOX_FEATURE_STATUS_INPUTS
        lockbox_status_inputs_t inputs = {0};
        if (status_inputs_read(&inputs) == ESP_OK) {
            lv_label_set_text(s_lid_label, inputs.lid_open ? "Lid: Open" : "Lid: Closed");
            lv_obj_set_style_text_color(s_lid_label,
                inputs.lid_open ? lv_color_hex(0xff8800) : lv_color_hex(0x44ff44), 0);
        }
#else
        lv_label_set_text(s_lid_label, "Lid: Closed");
        lv_obj_set_style_text_color(s_lid_label, lv_color_hex(0x44ff44), 0);
#endif
    }

    // Battery status — placeholder (green until battery service is implemented)
    if (s_battery_box && lv_obj_is_valid(s_battery_box)) {
        lv_obj_set_style_bg_color(s_battery_box, lv_color_hex(0x44cc44), 0);
    }
}

static void unlock_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    esp_err_t err = lock_service_unlock(NULL);
    if (err == ESP_OK) {
        if (s_lock_label && lv_obj_is_valid(s_lock_label)) {
            lv_label_set_text(s_lock_label, "Box: Unlocked");
            lv_obj_set_style_text_color(s_lock_label, lv_color_hex(0x44ff44), 0);
        }
        create_toast("Box Unlocked", 2000);
    } else {
        create_toast("Unlock Failed (servo error)", 3000);
        ESP_LOGE(SCREEN_TAG, "lock_service_unlock failed: %s", esp_err_to_name(err));
    }
}

static void change_pin_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_screen_change_pin_set_context(session_service_get_userid(), false);
    lv_scr_load_anim(ui_screen_change_pin, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
}

static void register_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ui_screen_enroll_set_context(session_service_get_userid(), false);
    lv_scr_load_anim(ui_screen_enroll, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
}

static void signout_btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    lock_service_lock(NULL);
    session_service_clear();
    lv_scr_load_anim(ui_screen_main, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 500, 0, false);
}

static void admin_button_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Admin button clicked, showing user management UI");
        user_mgmt_ui_show();
    }
}
