/**
 * @file ui_screen_rtc_date.c
 * @brief Screen for admin to set DS3231 RTC date-time (dd-mm-yyyy hh:mm + AM/PM).
 *        Uses a PIN-like virtual keypad (button_matrix) for date-time entry.
 *        
 * Fields: day (01-31), month (01-12), year (2000-2099),
 *         hour (01-12), minute (00-59), AM/PM toggle
 * Seconds always passed as 0 to RTC.
 */

#include "ui_screen_rtc_date.h"
#include "lvgl.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "services/ds3231_service.h"
#include "ui/ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#if !CONFIG_LOCKBOX_FEATURE_DS3231
void ui_screen_rtc_date_create(void)
{
    ESP_LOGW("UI_SCREEN_RTC", "DS3231 feature not enabled — screen not created");
}
void ui_screen_rtc_date_hide(void) {}
lv_obj_t *ui_screen_rtc_date_get(void) { return NULL; }
#else

static const char *SCREEN_TAG = "UI_SCREEN_RTC";

/* ── Field definitions — order of entry ── */

typedef enum {
    FIELD_DAY = 0,
    FIELD_MONTH = 1,
    FIELD_YEAR = 2,
    FIELD_HOUR = 3,
    FIELD_MINUTE = 4,
    FIELD_COUNT
} rtc_field_idx_t;

typedef struct {
    char digits[8];
    uint8_t  max_digits;
    int      min_val, max_val;
} rtc_field_t;

/* ── State ── */
static int s_focus_field = FIELD_DAY;
static bool  s_is_pm     = false;

/* ── Widgets ── */
static lv_obj_t *s_overlay     = NULL;
static lv_obj_t *s_input_label  = NULL;
static lv_obj_t *s_date_ind     = NULL;
static lv_obj_t *s_time_ind     = NULL;
static lv_obj_t *s_am_checkbox  = NULL;
static lv_obj_t *s_pm_checkbox  = NULL;
static lv_obj_t *s_err_lbl      = NULL;
static lv_obj_t *s_ok_lbl       = NULL;
static lv_obj_t *s_btn_apply    = NULL;
static lv_obj_t *s_btnmatrix    = NULL;

static rtc_field_t s_fields[FIELD_COUNT] = {
    [FIELD_DAY]    = { .digits = "", .max_digits = 2, .min_val = 1,   .max_val = 31  },
    [FIELD_MONTH]  = { .digits = "", .max_digits = 2, .min_val = 1,   .max_val = 12  },
    [FIELD_YEAR]   = { .digits = "", .max_digits = 4, .min_val = 2000,.max_val = 2099 },
    [FIELD_HOUR]   = { .digits = "", .max_digits = 2, .min_val = 1,   .max_val = 12  },
    [FIELD_MINUTE] = { .digits = "", .max_digits = 2, .min_val = 0,   .max_val = 59  },
};

/* ── Forward declarations ── */
static void update_display(void);
static void update_all_indicators(void);
static void apply_timer_cb(lv_timer_t *timer);
static void rtc_hide_timer_cb(lv_timer_t *timer);
static void toast_delete_timer_cb(lv_timer_t *timer);
static void create_toast(const char *text, int timeout_ms);
static void load_rtc_datetime(void);

/* ── Helpers ── */

static int field_value(int idx)
{
    if (s_fields[idx].digits[0] == '\0') return 0;
    return atoi(s_fields[idx].digits);
}

static int field_filled(int idx)
{
    return (int)strlen(s_fields[idx].digits);
}

static bool is_field_full(int idx)
{
    return (uint8_t)strlen(s_fields[idx].digits) >= s_fields[idx].max_digits;
}

static void focus_next(void)
{
    int started = s_focus_field;
    do {
        s_focus_field++;
        if (s_focus_field >= FIELD_COUNT)
            s_focus_field = FIELD_DAY;
        if (s_focus_field == started)
            break;               /* wrapped all the way */
    } while (is_field_full(s_focus_field));
    update_display();
}

static void backspace_current(void)
{
    /* If current field is empty, find the nearest previous non-empty field
     * and move focus there (but don't delete anything — let the next
     * backspace press on that field do the deletion). */
    int len = (int)strlen(s_fields[s_focus_field].digits);
    if (len == 0) {
        int prev = s_focus_field - 1;
        if (prev < 0)
            prev = FIELD_COUNT - 1;
        if (s_fields[prev].digits[0] != '\0') {
            s_focus_field = prev;
            len = (int)strlen(s_fields[s_focus_field].digits);
        }
    }
    /* Normal: backspace within current field */
    if (len > 0) {
        s_fields[s_focus_field].digits[len - 1] = '\0';
    }
    update_display();
}

static void unflash_date_ind(lv_timer_t *timer)
{
    (void)timer;
    if (s_date_ind)
        lv_obj_set_style_text_color(s_date_ind, lv_color_hex(0x00ff00), 0);
}

static void unflash_time_ind(lv_timer_t *timer)
{
    (void)timer;
    if (s_time_ind)
        lv_obj_set_style_text_color(s_time_ind, lv_color_hex(0x00ff00), 0);
}

static void insert_digit(char c)
{
    int len = (int)strlen(s_fields[s_focus_field].digits);
    if (len < s_fields[s_focus_field].max_digits) {
        s_fields[s_focus_field].digits[len] = c;
        s_fields[s_focus_field].digits[len + 1] = '\0';

        if (is_field_full(s_focus_field)) {
            int val = atoi(s_fields[s_focus_field].digits);
            bool ok = (val >= s_fields[s_focus_field].min_val &&
                       val <= s_fields[s_focus_field].max_val);
            if (!ok) {
                /* Flash the appropriate indicator based on field type */
                if (s_focus_field < FIELD_HOUR) {
                    /* Date fields flash the date indicator */
                    lv_obj_set_style_text_color(s_date_ind, lv_color_hex(0xff4444), 0);
                    lv_timer_t *t = lv_timer_create(unflash_date_ind, 1000, NULL);
                    lv_timer_set_repeat_count(t, 1);
                } else {
                    /* Time fields flash the time indicator */
                    lv_obj_set_style_text_color(s_time_ind, lv_color_hex(0xff4444), 0);
                    lv_timer_t *t = lv_timer_create(unflash_time_ind, 1000, NULL);
                    lv_timer_set_repeat_count(t, 1);
               }           }            focus_next();
        } else {
            update_display();
        }
    }
}

static void load_from_rtc(void)
{
    ds3231_datetime_t dt;
    if (ds3231_service_get_datetime(&dt) != ESP_OK) {
        /* Default: leave fields empty */
        return;
    }

    /* Convert 24h → 12h for display */
    int16_t hour12 = (int16_t)dt.time.hour;
    bool is_pm = false;
    if (hour12 == 0) {
        hour12 = 12; is_pm = false;  /* midnight → 12 AM */
    } else if (hour12 == 12) {
        is_pm = true;  /* noon → 12 PM */
    } else if (hour12 > 12) {
        hour12 -= 12; is_pm = true;
    }

    /* Populate field digits */
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d", (int)dt.date.day);
    strncpy(s_fields[FIELD_DAY].digits, buf, sizeof(s_fields[FIELD_DAY].digits) - 1);
    snprintf(buf, sizeof(buf), "%02d", (int)dt.date.month);
    strncpy(s_fields[FIELD_MONTH].digits, buf, sizeof(s_fields[FIELD_MONTH].digits) - 1);
    snprintf(buf, sizeof(buf), "%04d", (int)dt.date.year);
    strncpy(s_fields[FIELD_YEAR].digits, buf, sizeof(s_fields[FIELD_YEAR].digits) - 1);
    snprintf(buf, sizeof(buf), "%02d", hour12);
    strncpy(s_fields[FIELD_HOUR].digits, buf, sizeof(s_fields[FIELD_HOUR].digits) - 1);
    snprintf(buf, sizeof(buf), "%02d", (int)dt.time.minutes);
    strncpy(s_fields[FIELD_MINUTE].digits, buf, sizeof(s_fields[FIELD_MINUTE].digits) - 1);
    s_is_pm = is_pm;
}

static void clear_all_fields(void)
{
    for (int i = 0; i < FIELD_COUNT; i++)
        s_fields[i].digits[0] = '\0';
    s_focus_field = FIELD_DAY;
    s_is_pm = false;
    update_display();
}

/* ── AM/PM visual-sync helpers ── */

static void set_ampm_checked_from_var(void) {
    if (s_is_pm) {
        lv_obj_add_state(s_pm_checkbox, LV_STATE_CHECKED);
        lv_obj_clear_state(s_am_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_add_state(s_am_checkbox, LV_STATE_CHECKED);
        lv_obj_clear_state(s_pm_checkbox, LV_STATE_CHECKED);
    }
}

static void reset_all(void)
{
    load_from_rtc();
    /* Sync checkbox VISUAL states to the RTC-read time (guard for first-open when checkboxes are not yet created) */
    if (s_am_checkbox != NULL && s_pm_checkbox != NULL) {
        set_ampm_checked_from_var();
    }
    s_focus_field = FIELD_DAY;
    update_display();
}

/* ── Indicator helpers ── */

static void fill_field(char *out, const rtc_field_t *f)
{
    int len = (int)strlen(f->digits);
    for (int i = 0; i < (int)f->max_digits; i++) {
        *out++ = (i < len) ? f->digits[i] : '_';
    }
}

static void update_date_ind(void)
{
    char buf[20];
    memset(buf, 0, sizeof(buf));
    char *p = buf;
    fill_field(p, &s_fields[FIELD_DAY]); p += s_fields[FIELD_DAY].max_digits;
    *p++ = '/';
    fill_field(p, &s_fields[FIELD_MONTH]); p += s_fields[FIELD_MONTH].max_digits;
    *p++ = '/';
    fill_field(p, &s_fields[FIELD_YEAR]);  p += s_fields[FIELD_YEAR].max_digits;
    lv_label_set_text(s_date_ind, buf);
}

static void update_time_ind(void)
{
    char buf[20];
    memset(buf, 0, sizeof(buf));
    char *p = buf;
    fill_field(p, &s_fields[FIELD_HOUR]); p += s_fields[FIELD_HOUR].max_digits;
    *p++ = ':';
    fill_field(p, &s_fields[FIELD_MINUTE]); p += s_fields[FIELD_MINUTE].max_digits;
    lv_label_set_text(s_time_ind, buf);
}

static void update_all_indicators(void)
{
    update_date_ind();
    update_time_ind();
    /* AM/PM text-color is handled by LVGL checkbox styling (CHECKED → primary color, UNCHECKED → light) */
}

static void update_display(void)
{
    if (!s_input_label) return;
    const char *names[] = {"Day", "Month", "Year", "Hour", "Minute"};
    char buf[32];
    int off = snprintf(buf, sizeof(buf), "%s : ", names[s_focus_field]);
    fill_field(buf + off, &s_fields[s_focus_field]);
    lv_label_set_text(s_input_label, buf);
    update_all_indicators();
}

static void show_msg(lv_obj_t *lbl, const char *txt, lv_color_t clr)
{
    if (!lbl) return;
    lv_label_set_text(lbl, txt);
    lv_obj_set_style_text_color(lbl, clr, 0);
}
static void clear_msg(lv_obj_t *lbl)
{
    if (lbl) lv_label_set_text(lbl, "");
}

/* ── Toast ── */

static void toast_delete_timer_cb(lv_timer_t *timer)
{
    lv_obj_t *t = lv_timer_get_user_data(timer);
    lv_obj_del(t);
}

static void create_toast(const char *text, int timeout_ms)
{
    lv_obj_t *toast = lv_obj_create(lv_layer_top());
    lv_obj_add_flag(toast, LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(toast, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(toast, LV_ALIGN_TOP_MID, 0, 30);
    lv_obj_set_style_bg_color(toast, lv_color_hex(0xe19419), 0);
    lv_obj_set_style_radius(toast, 10, 0);
    lv_obj_set_style_pad_all(toast, 12, 0);

    lv_obj_t *lbl = lv_label_create(toast);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl);

    lv_timer_t *t = lv_timer_create(toast_delete_timer_cb, timeout_ms, toast);
    lv_timer_set_repeat_count(t, 1);
}

/* ── Timer callbacks (C, not lambdas) ── */

static void apply_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    ui_screen_rtc_date_hide();
}

static void rtc_hide_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    ui_screen_rtc_date_hide();
}

/* ── Event callbacks ── */

static void apply_button_event_cb(lv_event_t *e)
{
    (void)e;
    clear_msg(s_err_lbl);
    clear_msg(s_ok_lbl);

    /* All fields filled? */
    for (int i = 0; i < FIELD_COUNT; i++) {
        if (s_fields[i].digits[0] == '\0') {
            const char *names[] = {"Day", "Month", "Year", "Hour", "Minute"};
            char msg[64];
            snprintf(msg, sizeof(msg), "Enter all %s", names[i]);
            show_msg(s_err_lbl, msg, lv_color_hex(0xff6060));
            return;
        }
    }

    int day    = field_value(FIELD_DAY);
    int mon    = field_value(FIELD_MONTH);
    int year   = field_value(FIELD_YEAR);
    int hour   = field_value(FIELD_HOUR);
    int minute = field_value(FIELD_MINUTE);

    if (day < 1 || day > 31)    { show_msg(s_err_lbl, "Day 1-31", lv_color_hex(0xff6060)); return; }
    if (mon < 1 || mon > 12)    { show_msg(s_err_lbl, "Month 1-12", lv_color_hex(0xff6060)); return; }
    if (year < 2000 || year > 2099) { show_msg(s_err_lbl, "Year 2000-2099", lv_color_hex(0xff6060)); return; }
    if (hour < 1 || hour > 12)  { show_msg(s_err_lbl, "Hour 1-12", lv_color_hex(0xff6060)); return; }
    if (minute < 0 || minute > 59){ show_msg(s_err_lbl, "Minute 0-59", lv_color_hex(0xff6060)); return; }

    /* Convert 12h → 24h */
    int hour24 = hour;
    if (s_is_pm && hour != 12)
        hour24 = hour + 12;
    else if (!s_is_pm && hour == 12)
        hour24 = 0;

    ESP_LOGI(SCREEN_TAG, "RTC set: %02d/%02d/%04d %02d:%02d %s  → 24h %02d:%02d:00",
             day, mon, year, hour, minute, s_is_pm ? "PM" : "AM", hour24, minute);

    ds3231_datetime_t dt;
    memset(&dt, 0, sizeof(dt));
    dt.date.year   = (uint16_t)year;
    dt.date.month  = (uint8_t)mon;
    dt.date.day    = (uint8_t)day;
    dt.time.hour   = (uint8_t)hour24;
    dt.time.minutes = (uint8_t)minute;
    dt.time.seconds = 0;

    esp_err_t ret = ds3231_service_set_datetime(&dt);
    if (ret != ESP_OK) {
        show_msg(s_err_lbl, "Write failed!", lv_color_hex(0xff6060));
        ESP_LOGE(SCREEN_TAG, "DS3231 set_datetime failed: %s", esp_err_to_name(ret));
        return;
    }

    show_msg(s_ok_lbl, "RTC OK!", lv_color_hex(0x80ff80));
    ESP_LOGI(SCREEN_TAG, "RTC date-time OK: %04d-%02d-%02d %02d:%02d%s",
             year, mon, day, hour24, minute, s_is_pm ? "PM" : "AM");

    lv_timer_t *t = lv_timer_create(rtc_hide_timer_cb, 2000, NULL);
    lv_timer_set_repeat_count(t, 1);
}

static void cancel_button_event_cb(lv_event_t *e)
{
    (void)e;
    clear_msg(s_err_lbl);
    clear_msg(s_ok_lbl);
    ui_screen_rtc_date_hide();
    lv_obj_t *admin = ui_screen_user_mgmt_get();
    if (admin) lv_screen_load(admin);
}

static void ampm_toggle_event_cb(lv_event_t *e)
{
    /* VALUE_CHANGED fires AFTER LVGL toggles the clicked checkbox.
     * Determine which checkbox was clicked to set s_is_pm correctly. */
    lv_obj_t *target = lv_event_get_target(e);
    bool pm_clicked = (target == s_pm_checkbox);
    s_is_pm = pm_clicked ? true : false;

    /* Mutual exclusion: clear CHECKED on the opposite one */
    if (s_is_pm) {
        lv_obj_clear_state(s_am_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_pm_checkbox, LV_STATE_CHECKED);
    }

    /* Guard: ensure exactly-one is always checked.
     * If neither is checked (e.g. user clicked the only-checked AM box),
     * re-check AM as the default. */
    if (!lv_obj_has_state(s_am_checkbox, LV_STATE_CHECKED) &&
        !lv_obj_has_state(s_pm_checkbox, LV_STATE_CHECKED)) {
        lv_obj_add_state(s_am_checkbox, LV_STATE_CHECKED);
    }

    (void)e;
}


static void keypad_event_cb(lv_event_t *e)
{
    lv_obj_t *bm = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_VALUE_CHANGED) return;

    uint32_t btn_id = lv_buttonmatrix_get_selected_button(bm);
    const char *txt = lv_buttonmatrix_get_button_text(bm, btn_id);
    if (!txt) return;

    ESP_LOGI(SCREEN_TAG, "Keypad: '%s'", txt);

    if (txt[0] >= '0' && txt[0] <= '9' && txt[1] == '\0') {
        insert_digit(txt[0]);
    } else if (strcmp(txt, "Clear") == 0) {
        clear_all_fields();
    } else if (strcmp(txt, "Bksp") == 0) {
        backspace_current();
    }
}

/* ── Create overlay ── */

void ui_screen_rtc_date_create(void)
{
    if (s_overlay != NULL) {
        lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI(SCREEN_TAG, "RTC screen already exists — showing, reloading RTC");
        reset_all();       /* reload RTC values on each open */
        return;
    }

    ESP_LOGI(SCREEN_TAG, "Creating RTC date-time-set overlay");
    reset_all();

    /* ── Root floating overlay ── */
    s_overlay = lv_obj_create(lv_layer_top());
    if (s_overlay == NULL) {
        ESP_LOGE(SCREEN_TAG, "Failed to create overlay (lv_layer_top may be NULL)");
        return;
    }
    lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_size(s_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_center(s_overlay);
    lv_obj_clear_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_100, 0);
    lv_obj_set_style_border_width(s_overlay, 2, 0);
    lv_obj_set_style_border_color(s_overlay, lv_color_hex(0x555555), 0);
    lv_obj_set_style_radius(s_overlay, 12, 0);
    lv_obj_set_flex_flow(s_overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_overlay, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_overlay, 6, 0);

    /* ── Title ── */
    lv_obj_t *title = lv_label_create(s_overlay);
    lv_label_set_text(title, "Set Date-Time");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_width(title, LV_PCT(100), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(title, 6, 0);

    // /* ── date tip ── */
    // lv_obj_t *date_tip = lv_label_create(s_overlay);
    // lv_label_set_text(date_tip, "dd/mm/yyyy");
    // //lv_obj_set_pos(date_tip, 700, 200);
    // //lv_obj_set_size(date_tip, 100, 50);
    // lv_obj_set_style_text_font(date_tip, &lv_font_montserrat_24, 0);
    // lv_obj_set_style_text_color(date_tip, lv_color_hex(0xffffff), 0);
    // //lv_obj_align(date_tip, LV_ALIGN_TOP_RIGHT, -20, 400);
    // lv_obj_set_style_text_align(date_tip, LV_TEXT_ALIGN_RIGHT, 0);
    // lv_obj_set_style_width(date_tip, LV_PCT(80), 0);
    // lv_obj_set_style_pad_top(date_tip, 6, 0);

    // /* ── Date / Time indicators ── */
    // s_date_ind = lv_label_create(s_overlay);
    // lv_label_set_text(s_date_ind, "__ / __ / ______");
    // lv_obj_set_style_text_font(s_date_ind, &lv_font_montserrat_32, 0);
    // lv_obj_set_style_text_color(s_date_ind, lv_color_hex(0x00ff00), 0);
    // lv_obj_set_style_text_align(s_date_ind, LV_TEXT_ALIGN_CENTER, 0);
    // lv_obj_set_style_width(s_date_ind, LV_PCT(80), 0);

    /* ── Date indicator (keeps its flex slot / position unchanged) ── */
    s_date_ind = lv_label_create(s_overlay);
    lv_label_set_text(s_date_ind, "__ / __ / ____");
    lv_obj_set_style_text_font(s_date_ind, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(s_date_ind, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_align(s_date_ind, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_width(s_date_ind, LV_PCT(80), 0);

    /* ── date tip: floating, anchored to the right of date_ind ── */
    lv_obj_t *date_tip = lv_label_create(s_overlay);
    lv_label_set_text(date_tip, "dd/mm/yyyy");
    lv_obj_set_style_text_font(date_tip, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(date_tip, lv_color_hex(0xaaaaaa), 0);
    lv_obj_add_flag(date_tip, LV_OBJ_FLAG_FLOATING);                 /* no flex slot */
    lv_obj_align_to(date_tip, s_date_ind, LV_ALIGN_OUT_RIGHT_MID, -175, -202);  /* 12px gap */

    s_time_ind = lv_label_create(s_overlay);
    lv_label_set_text(s_time_ind, "__ : __");
    lv_obj_set_style_text_font(s_time_ind, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(s_time_ind, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_text_align(s_time_ind, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_width(s_time_ind, LV_PCT(80), 0);

    /* ── time tip: floating, anchored to the right of time_ind ── */
    lv_obj_t *time_tip = lv_label_create(s_overlay);
    lv_label_set_text(time_tip, "hh:mm");
    lv_obj_set_style_text_font(time_tip, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(time_tip, lv_color_hex(0xaaaaaa), 0);
    lv_obj_add_flag(time_tip, LV_OBJ_FLAG_FLOATING);                 /* no flex slot */
    lv_obj_align_to(time_tip, s_time_ind, LV_ALIGN_OUT_RIGHT_MID, -155, -180);  /* 12px gap */

/* ── AM/PM row ── */
    /* ── AM/PM row — more compact but maintain checkbox usability ── */
    lv_obj_t *ampm_row = lv_obj_create(s_overlay);
    lv_obj_set_size(ampm_row, LV_PCT(35), 40);   /* Reduced from 54px to 48px for better fit */
    lv_obj_clear_flag(ampm_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(ampm_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(ampm_row, 35, 0); /* 30px gap between checkboxes */
    lv_obj_set_flex_align(ampm_row, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_am_checkbox = lv_checkbox_create(ampm_row);
    lv_obj_set_size(s_am_checkbox, 68, LV_SIZE_CONTENT);       /* Keep original size for touch usability */
    lv_checkbox_set_text(s_am_checkbox, "AM");
    lv_obj_set_style_text_font(s_am_checkbox, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    s_pm_checkbox = lv_checkbox_create(ampm_row);
    lv_obj_set_size(s_pm_checkbox, 68, LV_SIZE_CONTENT);       /* Keep original size for touch usability */
    lv_checkbox_set_text(s_pm_checkbox, "PM");
    lv_obj_set_style_text_font(s_pm_checkbox, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(s_am_checkbox, ampm_toggle_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_pm_checkbox, ampm_toggle_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Sync AM/PM checkbox to the RTC-read time (first-open path) */
    set_ampm_checked_from_var();

    /* ── Input label ── */
    s_input_label = lv_label_create(s_overlay);
    lv_label_set_text(s_input_label, "Day : __");
    lv_obj_set_style_text_font(s_input_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(s_input_label, lv_color_hex(0xffaa00), 0);
    lv_obj_set_style_text_align(s_input_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_width(s_input_label, LV_PCT(80), 0);

    /* ── Error / OK labels ── */
    s_err_lbl = lv_label_create(s_overlay);
    lv_label_set_text(s_err_lbl, "");
    lv_obj_set_style_text_font(s_err_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_err_lbl, lv_color_hex(0xff6060), 0);
    lv_obj_set_style_text_align(s_err_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_width(s_err_lbl, LV_PCT(75), 0);

    s_ok_lbl = lv_label_create(s_overlay);
    lv_label_set_text(s_ok_lbl, "");
    lv_obj_set_style_text_font(s_ok_lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_ok_lbl, lv_color_hex(0x80ff80), 0);
    lv_obj_set_style_text_align(s_ok_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_width(s_ok_lbl, LV_PCT(75), 0);

    /* ── Keypad (button_matrix) ── */
    s_btnmatrix = lv_buttonmatrix_create(s_overlay);
    lv_obj_set_size(s_btnmatrix, LV_PCT(70), 190);
    lv_obj_set_style_text_font(s_btnmatrix, &lv_font_montserrat_22, 0);
    static const char *kp_map[] = {
        "1", "2", "3", "\n",
        "4", "5", "6", "\n",
        "7", "8", "9", "\n",
        "Clear", "0", "Bksp",  /* Backspace */
        "",                         /* spacer */
        NULL
    };
    lv_buttonmatrix_set_map(s_btnmatrix, kp_map);
    lv_obj_add_event_cb(s_btnmatrix, keypad_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* ── Buttons ── */
    lv_obj_t *btn_row = lv_obj_create(s_overlay);
    lv_obj_set_size(btn_row, LV_PCT(70), 40);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_btn_apply = lv_button_create(btn_row);
    lv_obj_set_size(s_btn_apply, 100, 36);
    lv_obj_set_style_bg_color(s_btn_apply, lv_color_hex(0x007800), 0);
    lv_obj_set_style_radius(s_btn_apply, 6, 0);
    lv_obj_t *lbl_a = lv_label_create(s_btn_apply);
    lv_label_set_text(lbl_a, "APPLY");
    lv_obj_set_style_text_color(lbl_a, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_a);
    lv_obj_add_event_cb(s_btn_apply, apply_button_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_c = lv_button_create(btn_row);
    lv_obj_set_size(btn_c, 100, 36);
    lv_obj_set_style_bg_color(btn_c, lv_color_hex(0x444444), 0);
    lv_obj_set_style_radius(btn_c, 6, 0);
    lv_obj_t *lbl_c = lv_label_create(btn_c);
    lv_label_set_text(lbl_c, "Cancel");
    lv_obj_set_style_text_color(lbl_c, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_c);
    lv_obj_add_event_cb(btn_c, cancel_button_event_cb, LV_EVENT_CLICKED, NULL);

    update_display();
    ESP_LOGI(SCREEN_TAG, "RTC screen created (ready for input)");
}

void ui_screen_rtc_date_hide(void)
{
    if (s_overlay) lv_obj_add_flag(s_overlay, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t *ui_screen_rtc_date_get(void)
{
    return s_overlay;
}

#endif /* CONFIG_LOCKBOX_FEATURE_DS3231 */
