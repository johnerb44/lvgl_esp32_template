---
description: "Use when creating or modifying UI screens, dialogs, toasts, or overlays in main/ui/. Covers screen file conventions, toast notification pattern, confirm dialog pattern, and keyboard/text-entry overlay pattern for this LVGL 9.3 project."
applyTo: "main/ui/**"
---

# UI Screen Conventions

## Screen File Structure

Every screen lives in `main/ui/screens/ui_screen_<name>.c/.h`. Required public interface:

```c
// ui_screen_<name>.h
void ui_screen_<name>_create(void);
lv_obj_t *ui_screen_<name>_get(void);
```

Registration checklist:
1. Call `ui_screen_<name>_create()` inside `ui_init()` in `ui/ui.c`
2. Declare `extern lv_obj_t *ui_screen_<name>;` in `ui/ui.h`
3. Add `ui/screens/ui_screen_<name>.c` to `SRCS` in `main/CMakeLists.txt`

File-local objects use `static lv_obj_t *s_*` naming. Per-file log tag: `static const char *SCREEN_TAG = "UI_SCREEN_NAME"`.

## Screen Navigation

```c
lv_screen_load(ui_screen_home);        // navigate to a screen
lv_obj_t *cur = lv_screen_active();   // get current screen
```

## Toast Notification Pattern

```c
static void cb_delete_toast(lv_timer_t *timer) {
    lv_obj_del((lv_obj_t *)lv_timer_get_user_data(timer));
}

static void show_toast(const char *msg) {
    lv_obj_t *toast = lv_obj_create(lv_layer_top());
    lv_obj_add_flag(toast, LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(toast, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(toast, LV_ALIGN_BOTTOM_MID, 0, -30);

    lv_obj_t *label = lv_label_create(toast);
    lv_label_set_text(label, msg);

    lv_timer_t *t = lv_timer_create(cb_delete_toast, 2000, toast);
    lv_timer_set_repeat_count(t, 1);
}
```

Always parent on `lv_layer_top()`, never the screen — it survives screen transitions and closes over any other content.

## Confirm Dialog Pattern

```c
static void cb_confirm(lv_event_t *e) {
    lv_obj_t *mbox = lv_obj_get_parent(lv_event_get_target(e));
    const char *btn = lv_msgbox_get_active_btn_text(mbox);
    if (strcmp(btn, "OK") == 0) {
        // perform action
    }
    lv_msgbox_close(mbox);
}

static void show_confirm(const char *title, const char *msg) {
    lv_obj_t *mbox = lv_msgbox_create(lv_layer_top());
    lv_msgbox_add_title(mbox, title);
    lv_msgbox_add_text(mbox, msg);
    lv_obj_t *cancel_btn = lv_msgbox_add_footer_button(mbox, "Cancel");
    lv_obj_t *ok_btn = lv_msgbox_add_footer_button(mbox, "OK");
    lv_obj_add_event_cb(ok_btn, cb_confirm, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(cancel_btn, cb_confirm, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(mbox, 300, 180);
    lv_obj_center(mbox);
}
```

## Keyboard / Text-Entry Overlay Pattern

Used for inline editing (e.g., username, PIN entry without a dedicated screen):

```c
static lv_obj_t *s_edit_overlay = NULL;

static void open_edit_overlay(const char *current_val, bool is_password) {
    s_edit_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_edit_overlay, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(s_edit_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *ta = lv_textarea_create(s_edit_overlay);
    lv_textarea_set_password_mode(ta, is_password);
    lv_textarea_set_text(ta, current_val ? current_val : "");
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *kb = lv_keyboard_create(s_edit_overlay);
    lv_keyboard_set_mode(kb, is_password ? LV_KEYBOARD_MODE_NUMBER : LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_textarea(kb, ta);
}

static void close_edit_overlay(void) {
    if (s_edit_overlay) {
        lv_obj_del(s_edit_overlay);
        s_edit_overlay = NULL;
    }
}
```

## Async Operations (background task → UI update)

Never call `lv_*` from a FreeRTOS task. Post results back via a one-shot timer:

```c
static void cb_result_timer(lv_timer_t *timer) {
    my_result_t *result = lv_timer_get_user_data(timer);
    // update UI with result
    free(result);
}

// inside FreeRTOS task:
my_result_t *result = malloc(sizeof(*result));
*result = computed_result;
lv_timer_t *t = lv_timer_create(cb_result_timer, 0, result);
lv_timer_set_repeat_count(t, 1);
vTaskDelete(NULL);
```

## Style Selectors

Third argument to `lv_obj_set_style_*()` is a part+state selector:

```c
lv_obj_set_style_bg_color(obj, color, 0);                          // default part+state
lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN | LV_STATE_DEFAULT);  // explicit
lv_obj_set_style_text_color(obj, color, LV_STATE_CHECKED);         // checked state only
```
