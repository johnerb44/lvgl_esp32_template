/**
 * @file ui_screen_rtc_date.c
 * @brief Screen for admin to set DS3231 RTC date (dd-mm-yyyy).
 *        Uses lv_roller widgets for touch-based scroll selection.
 */

#include "ui_screen_rtc_date.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "services/ds3231_service.h"
#include "ch422g_driver.h"
#include "ui/ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#if !CONFIG_LOCKBOX_FEATURE_DS3231
/* RTC date set screen disabled when feature not enabled */
void ui_screen_rtc_date_create(void)
{
    ESP_LOGW("UI_SCREEN_RTC", "DS3231 feature not enabled — screen not created");
}

void ui_screen_rtc_date_hide(void)
{
    /* no-op when disabled */
}

#else /* CONFIG_LOCKBOX_FEATURE_DS3231 */

static const char *SCREEN_TAG = "UI_SCREEN_RTC";

/* --- Widgets --- */
static lv_obj_t *s_rtc_screen = NULL;   /* RTC date-set overlay (created at first show) */
static lv_obj_t *s_btn_refresh = NULL;   /* Refresh date button */
static lv_obj_t *s_apply_btn = NULL;     /* Apply date button */
static lv_obj_t *s_day_roller = NULL;   /* Day (01-31) */
static lv_obj_t *s_mon_roller = NULL;   /* Month (01-12) */
static lv_obj_t *s_year_roller = NULL;  /* Year (2000-2099) */
static lv_obj_t *s_err_label = NULL;    /* Error message label */
static lv_obj_t *s_ok_label = NULL;     /* Success message label */

/* --- Helpers --- */

/** Build options string for day roller: "01\n02\n...\n31" */
static void build_options_day(char *buf, size_t len)
{
    buf[0] = '\0';
    for (int i = 1; i <= 31; i++) {
        int written = snprintf(buf + strlen(buf), len - strlen(buf), "%02d", i);
        if (written > 0 && strlen(buf) < len - 1) {
            strncat(buf, "\n", len - strlen(buf) - 1);
        }
    }
}

/** Build options string for month roller: "01\n02\n...\n12" */
static void build_options_mon(char *buf, size_t len)
{
    buf[0] = '\0';
    for (int i = 1; i <= 12; i++) {
        int written = snprintf(buf + strlen(buf), len - strlen(buf), "%02d", i);
        if (written > 0 && strlen(buf) < len - 1) {
            strncat(buf, "\n", len - strlen(buf) - 1);
        }
    }
}

/**
 * Build options string for year roller with circular wrap.
 * Order: 2000, 2001, ..., current_year-1, current_year, ..., 2099
 * LV_ROLLER_MODE_CIRCULAR shows 3 rows: prev, selected, next.
 * @param buf        Output buffer (must be large enough)
 * @param len        Buffer length
 * @param current_year  The year to initially select in this roller
 */
static void build_options_year(char *buf, size_t len, uint16_t current_year)
{
    buf[0] = '\0';
    /* Years before current (for circular wrap-around) */
    for (uint16_t y = 2000; y < current_year; y++) {
        snprintf(buf + strlen(buf), len - strlen(buf), "%04d\n", y);
    }
    /* All years from current through 2099 */
    for (uint16_t y = current_year; y <= 2099; y++) {
        snprintf(buf + strlen(buf), len - strlen(buf), "%04d\n", y);
    }
}

/** Get current RTC date and use it for initial selection */
static void get_initial_date(uint16_t *p_day, uint16_t *p_mon, uint16_t *p_year)
{
    ds3231_date_t d = {0};
    if (ds3231_service_get_date(&d) == ESP_OK) {
        *p_day   = d.day;
        *p_mon   = d.month;
        *p_year  = d.year;
    } else {
        *p_day   = 1;
        *p_mon   = 1;
        *p_year  = 2026;
    }
}

/** Set the roller selections to match the current date */
static void set_roller_from_date(uint8_t day, uint8_t mon, uint16_t year)
{
    if (s_day_roller) {
        int idx = day - 1;
        if (idx < 0) idx = 0;
        if (idx >= 31) idx = 30;
        lv_roller_set_selected(s_day_roller, (uint32_t)idx, LV_ANIM_OFF);
    }
    if (s_mon_roller) {
        int idx = mon - 1;
        if (idx < 0) idx = 0;
        if (idx >= 12) idx = 11;
        lv_roller_set_selected(s_mon_roller, (uint32_t)idx, LV_ANIM_OFF);
    }
    if (s_year_roller) {
        /* Year options are built as: 2000, 2001, ... current_year-1, current_year, ... 2099
         * So index = year - 2000 always works */
        int idx = (int)(year - 2000);
        if (idx < 0) idx = 0;
        lv_roller_set_selected(s_year_roller, (uint32_t)idx, LV_ANIM_OFF);
    }
}

/* Forward declarations */
static void rtc_screen_hide_timer_cb(lv_timer_t *timer);
static void apply_button_event_cb(lv_event_t *e);
static void cancel_button_event_cb(lv_event_t *e);
static void refresh_button_event_cb(lv_event_t *e);

/* ========== Create RTC date-set overlay ========== */

void ui_screen_rtc_date_create(void)
{
    if (s_rtc_screen != NULL) {
        lv_obj_clear_flag(s_rtc_screen, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(SCREEN_TAG, "RTC screen already exists — showing");
        return;
    }

    ESP_LOGI(SCREEN_TAG, "Creating RTC date-set overlay");

    /* --- Root floating overlay --- */
    s_rtc_screen = lv_obj_create(lv_layer_top());
    lv_obj_add_flag(s_rtc_screen, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(s_rtc_screen, LV_PCT(85), LV_PCT(65));
    lv_obj_center(s_rtc_screen);
    lv_obj_set_style_bg_color(s_rtc_screen, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_bg_opa(s_rtc_screen, LV_OPA_100, 0);
    lv_obj_set_style_border_width(s_rtc_screen, 2, 0);
    lv_obj_set_style_border_color(s_rtc_screen, lv_color_hex(0x555555), 0);
    lv_obj_set_style_radius(s_rtc_screen, 12, 0);
    lv_obj_set_flex_flow(s_rtc_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_rtc_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_rtc_screen, 6, 0);

    /* --- Title --- */
    lv_obj_t *title = lv_label_create(s_rtc_screen);
    lv_label_set_text(title, "Set RTC Date");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_width(title, LV_PCT(100), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(title, 16, 0);

    /* --- Info label --- */
    lv_obj_t *info = lv_label_create(s_rtc_screen);
    lv_label_set_text(info, "Scroll rollers then press Apply");
    lv_obj_set_style_text_font(info, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(info, lv_color_hex(0x888888), 0);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_width(info, LV_PCT(90), 0);
    lv_obj_set_style_pad_bottom(info, 4, 0);

    /* --- Day picker --- */
    lv_obj_t *cont = lv_obj_create(s_rtc_screen);
    lv_obj_set_size(cont, LV_PCT(75), 36);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, 6, 0);

    lv_obj_t *lbl = lv_label_create(cont);
    lv_label_set_text(lbl, "Day:");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xcccccc), 0);

    /* Build day options and create roller */
    char day_opts[256];
    build_options_day(day_opts, sizeof(day_opts));
    s_day_roller = lv_roller_create(cont);
    lv_roller_set_options(s_day_roller, day_opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_day_roller, 1);
    lv_obj_set_style_width(s_day_roller, 50, 0);
    lv_obj_set_style_text_align(s_day_roller, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(s_day_roller, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_day_roller, lv_color_hex(0x3a3a3a), LV_PART_SELECTED);
    lv_obj_set_style_radius(s_day_roller, 4, 0);

    /* --- Month picker --- */
    cont = lv_obj_create(s_rtc_screen);
    lv_obj_set_size(cont, LV_PCT(75), 36);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, 6, 0);

    lbl = lv_label_create(cont);
    lv_label_set_text(lbl, "Month:");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xcccccc), 0);

    char mon_opts[128];
    build_options_mon(mon_opts, sizeof(mon_opts));
    s_mon_roller = lv_roller_create(cont);
    lv_roller_set_options(s_mon_roller, mon_opts, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(s_mon_roller, 1);
    lv_obj_set_style_width(s_mon_roller, 50, 0);
    lv_obj_set_style_text_align(s_mon_roller, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(s_mon_roller, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_mon_roller, lv_color_hex(0x3a3a3a), LV_PART_SELECTED);
    lv_obj_set_style_radius(s_mon_roller, 4, 0);

    /* --- Year picker --- */
    cont = lv_obj_create(s_rtc_screen);
    lv_obj_set_size(cont, LV_PCT(75), 36);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(cont, 6, 0);

    lbl = lv_label_create(cont);
    lv_label_set_text(lbl, "Year:");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xcccccc), 0);

    /* Year roller (circular: 2000...year-1, year, year+1...2099) */
    uint16_t current_year = 2026;
    char year_opts[2048];
    build_options_year(year_opts, sizeof(year_opts), current_year);
    s_year_roller = lv_roller_create(cont);
    lv_roller_set_options(s_year_roller, year_opts, LV_ROLLER_MODE_INFINITE);
    uint32_t current_idx = (uint32_t)(current_year - 2000); /* 26 for 2026 */
    lv_roller_set_selected(s_year_roller, current_idx, LV_ANIM_OFF);
    lv_obj_set_style_width(s_year_roller, 64, 0);
    lv_obj_set_style_text_align(s_year_roller, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(s_year_roller, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(s_year_roller, lv_color_hex(0x3a3a3a), LV_PART_SELECTED);
    lv_obj_set_style_radius(s_year_roller, 4, 0);

    /* --- Error label --- */
    s_err_label = lv_label_create(s_rtc_screen);
    lv_label_set_text(s_err_label, "");
    lv_obj_set_style_text_font(s_err_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_err_label, lv_color_hex(0xff6060), 0);
    lv_obj_set_style_text_align(s_err_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_width(s_err_label, LV_PCT(75), 0);
    lv_obj_set_style_pad_top(s_err_label, 4, 0);

    /* --- OK label --- */
    s_ok_label = lv_label_create(s_rtc_screen);
    lv_label_set_text(s_ok_label, "");
    lv_obj_set_style_text_font(s_ok_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ok_label, lv_color_hex(0x80ff80), 0);
    lv_obj_set_style_text_align(s_ok_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_width(s_ok_label, LV_PCT(75), 0);
    lv_obj_set_style_pad_top(s_ok_label, 2, 0);

    /* --- Button row --- */
    lv_obj_t *btn_row = lv_obj_create(s_rtc_screen);
    lv_obj_set_size(btn_row, LV_PCT(75), 36);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_row, 10, 0);

    /* Refresh date button */
    lv_obj_t *btn_refresh = lv_button_create(btn_row);
    lv_obj_set_size(btn_refresh, 48, 32);
    lv_obj_set_style_bg_color(btn_refresh, lv_color_hex(0x1e6bb5), 0);
    lv_obj_set_style_radius(btn_refresh, 4, 0);
    lv_obj_t *lbl_refresh = lv_label_create(btn_refresh);
    lv_label_set_text(lbl_refresh, LV_SYMBOL_REFRESH " R");
    lv_obj_center(lbl_refresh);
    lv_obj_add_event_cb(btn_refresh, refresh_button_event_cb, LV_EVENT_CLICKED, NULL);

    /* Apply button */
    s_apply_btn = lv_button_create(btn_row);
    lv_obj_set_size(s_apply_btn, 70, 32);
    lv_obj_set_style_bg_color(s_apply_btn, lv_color_hex(0x007800), 0);
    lv_obj_set_style_radius(s_apply_btn, 4, 0);
    lv_obj_t *lbl_apply = lv_label_create(s_apply_btn);
    lv_label_set_text(lbl_apply, "Apply");
    lv_obj_center(lbl_apply);
    lv_obj_add_event_cb(s_apply_btn, apply_button_event_cb, LV_EVENT_CLICKED, NULL);

    /* Cancel button */
    lv_obj_t *btn_cancel = lv_button_create(btn_row);
    lv_obj_set_size(btn_cancel, 70, 32);
    lv_obj_set_style_bg_color(btn_cancel, lv_color_hex(0x444444), 0);
    lv_obj_set_style_radius(btn_cancel, 4, 0);
    lv_obj_t *lbl_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(lbl_cancel, "Cancel");
    lv_obj_center(lbl_cancel);
    lv_obj_add_event_cb(btn_cancel, cancel_button_event_cb, LV_EVENT_CLICKED, NULL);

    /* --- Set initial roller selections from current RTC date --- */
    uint16_t day_val = 1, mon_val = 1, year_val = 2026;
    get_initial_date(&day_val, &mon_val, &year_val);
    set_roller_from_date((uint8_t)day_val, (uint8_t)mon_val, (uint16_t)year_val);

    ESP_LOGI(SCREEN_TAG, "RTC screen created (day=%d, mon=%d, year=%d)",
             day_val, mon_val, year_val);
}

void ui_screen_rtc_date_hide(void)
{
    if (s_rtc_screen != NULL) {
        lv_obj_add_flag(s_rtc_screen, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_screen_rtc_date_get(void)
{
    return s_rtc_screen;
}

/* ======================================================================
 * Event callbacks
 * ====================================================================== */

static void refresh_button_event_cb(lv_event_t *e)
{
    (void)e;
    /* Hide success/error messages */
    if (s_ok_label) lv_label_set_text(s_ok_label, "");
    if (s_err_label) lv_label_set_text(s_err_label, "");

    /* Read current RTC date and update rollers */
    ds3231_date_t cur_date = {0};
    if (ds3231_service_get_date(&cur_date) == ESP_OK) {
        ESP_LOGI(SCREEN_TAG, "Refetched RTC date: day=%d mon=%d year=%d",
                 cur_date.day, cur_date.month, cur_date.year);
        set_roller_from_date(cur_date.day, cur_date.month, cur_date.year);
    } else {
        ESP_LOGW(SCREEN_TAG, "Failed to read RTC date (maybe DS3231 not installed?)");
        if (s_err_label) lv_label_set_text(s_err_label, "DS3231 not found");
    }
}

static void apply_button_event_cb(lv_event_t *e)
{
    (void)e;

    /* Clear messages */
    if (s_ok_label) lv_label_set_text(s_ok_label, "");
    if (s_err_label) lv_label_set_text(s_err_label, "");

    /* --- Read roller values --- */
    uint32_t day_idx   = lv_roller_get_selected(s_day_roller);
    uint32_t mon_idx   = lv_roller_get_selected(s_mon_roller);
    uint32_t year_idx  = lv_roller_get_selected(s_year_roller);

    uint16_t day   = (uint16_t)(day_idx + 1);
    uint16_t mon   = (uint16_t)(mon_idx + 1);
    /* Year options are 2000, 2001, ..., 2099 so index 0 = 2000 */
    uint16_t year  = (uint16_t)(year_idx + 2000);

    ESP_LOGI(SCREEN_TAG, "Selected: day=%u mon=%u year=%u (idx: day=%lu mon=%lu year=%lu)",
             (unsigned)day, (unsigned)mon, (unsigned)year,
             (unsigned long)day_idx, (unsigned long)mon_idx, (unsigned long)year_idx);

    /* --- Validate --- */
    if (day < 1 || day > 31) {
        if (s_err_label) lv_label_set_text(s_err_label, "Day out of range");
        return;
    }
    if (mon < 1 || mon > 12) {
        if (s_err_label) lv_label_set_text(s_err_label, "Month out of range");
        return;
    }
    if (year < 2000 || year > 2099) {
        if (s_err_label) lv_label_set_text(s_err_label, "Year out of range");
        return;
    }

    /* --- Write to DS3231 --- */
    esp_err_t ret = ds3231_service_set_date_only((uint8_t)day, (uint8_t)mon, year);
    if (ret != ESP_OK) {
        if (s_err_label) lv_label_set_text(s_err_label, "Write failed!");
        ESP_LOGE(SCREEN_TAG, "DS3231 set_date failed: %s", esp_err_to_name(ret));
        return;
    }

    /* --- Success --- */
    if (s_ok_label)
        lv_label_set_text_fmt(s_ok_label, "OK! %02d-%02d-%04d",
                             (int)day, (int)mon, (int)year);

    ESP_LOGI(SCREEN_TAG, "RTC date set to %02d-%02d-%04d",
             (int)day, (int)mon, (int)year);

    /* Auto-hide RTC screen after 2 s */
    lv_timer_t *t = lv_timer_create(rtc_screen_hide_timer_cb, 2000, NULL);
    lv_timer_set_repeat_count(t, 1);
}

static void cancel_button_event_cb(lv_event_t *e)
{
    (void)e;

    /* Clear success/error messages */
    if (s_ok_label) lv_label_set_text(s_ok_label, "");
    if (s_err_label) lv_label_set_text(s_err_label, "");

    /* Hide RTC overlay */
    if (s_rtc_screen) {
        lv_obj_add_flag(s_rtc_screen, LV_OBJ_FLAG_HIDDEN);
    }

    /* Navigate to admin (user management) screen */
    lv_obj_t *admin_scr = ui_screen_user_mgmt_get();
    if (admin_scr) lv_screen_load(admin_scr);
}

/* Timer callback: auto-hide RTC overlay */
static void rtc_screen_hide_timer_cb(lv_timer_t *timer)
{
    lv_timer_del(timer);
    if (s_rtc_screen) lv_obj_add_flag(s_rtc_screen, LV_OBJ_FLAG_HIDDEN);
}

#endif /* CONFIG_LOCKBOX_FEATURE_DS3231 */
