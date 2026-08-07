#ifndef UI_SCREEN_ENROLL_H
#define UI_SCREEN_ENROLL_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_screen_enroll_create(void);
void ui_screen_enroll_set_context(int userid, bool from_first_time_setup);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_ENROLL_H
