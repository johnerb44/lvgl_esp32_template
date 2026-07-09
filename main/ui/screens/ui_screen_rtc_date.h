/**
 * @file ui_screen_rtc_date.h
 * @brief Screen for admin to set DS3231 RTC date
 */

#ifndef UI_SCREEN_RTC_DATE_H
#define UI_SCREEN_RTC_DATE_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create the RTC date-set screen and navigate to it
 */
void ui_screen_rtc_date_create(void);

/**
 * @brief Get the root object of the screen
 */
lv_obj_t *ui_screen_rtc_date_get(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_SCREEN_RTC_DATE_H */
