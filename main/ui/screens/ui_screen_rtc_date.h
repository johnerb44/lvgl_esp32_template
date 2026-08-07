/**
 * @file ui_screen_rtc_date.h
 * @brief Screen for admin to set DS3231 RTC date-time (dd-mm-yyyy hh:mm + AM/PM).
 *        Uses a PIN-like virtual keypad (button_matrix) for date-time entry.
 */

#ifndef UI_SCREEN_RTC_DATE_H
#define UI_SCREEN_RTC_DATE_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create the RTC date-time-set overlay and show it
 */
void ui_screen_rtc_date_create(void);

/**
 * @brief Hide the RTC date-time-set overlay
 */
void ui_screen_rtc_date_hide(void);

/**
 * @brief Get the root object of the screen
 */
lv_obj_t *ui_screen_rtc_date_get(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_SCREEN_RTC_DATE_H */
