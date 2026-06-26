# LVGL v8 → v9 API Mapping

## Display

| v8 | v9 |
|----|----|
| `lv_disp_t` | `lv_display_t` |
| `lv_disp_drv_t` | removed; use `lv_display_create()` directly |
| `lv_disp_drv_init(&drv)` | removed |
| `lv_disp_drv_register(&drv)` | `lv_display_create(w, h)` |
| `lv_disp_draw_buf_init(buf, p1, p2, n)` | `lv_display_set_buffers(disp, p1, p2, n, mode)` |
| `lv_disp_set_flush_cb(drv, cb)` | `lv_display_set_flush_cb(disp, cb)` |
| `lv_disp_flush_ready(drv)` | `lv_display_flush_ready(disp)` |
| `lv_disp_get_default()` | `lv_display_get_default()` |
| `lv_disp_get_hor_res(disp)` | `lv_display_get_horizontal_resolution(disp)` |
| `lv_disp_get_ver_res(disp)` | `lv_display_get_vertical_resolution(disp)` |
| `lv_disp_set_user_data(disp, ptr)` | `lv_display_set_user_data(disp, ptr)` |
| `lv_disp_get_user_data(disp)` | `lv_display_get_user_data(disp)` |

## Screen / Object Navigation

| v8 | v9 |
|----|----|
| `lv_disp_load_scr(scr)` | `lv_screen_load(scr)` |
| `lv_disp_get_scr_act(disp)` | `lv_screen_active()` |
| `lv_scr_act()` | `lv_screen_active()` |
| `lv_layer_top()` | `lv_layer_top()` *(unchanged)* |
| `lv_obj_get_disp(obj)` | `lv_obj_get_display(obj)` |

## Input Device

| v8 | v9 |
|----|----|
| `lv_indev_drv_t` | removed |
| `lv_indev_drv_init(&drv)` | removed |
| `lv_indev_drv_register(&drv)` | `lv_indev_create()` + `lv_indev_set_type()` + `lv_indev_set_read_cb()` |
| `drv.type = LV_INDEV_TYPE_POINTER` | `lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER)` |
| `drv.read_cb = my_read` | `lv_indev_set_read_cb(indev, my_read)` |
| `lv_indev_get_user_data(indev)` | `lv_indev_get_user_data(indev)` *(unchanged)* |

## Styles

| v8 | v9 |
|----|----|
| `lv_obj_set_style_*(obj, val, LV_STATE_DEFAULT)` | `lv_obj_set_style_*(obj, val, 0)` or `LV_PART_MAIN \| LV_STATE_DEFAULT` |
| `lv_style_set_*(style, val)` *(bare state)* | Add explicit part selector when applying: `lv_obj_add_style(obj, &style, LV_PART_MAIN)` |

## Draw Buffer Mode

| v8 constant | v9 equivalent |
|-------------|---------------|
| *(single buf)* | `LV_DISPLAY_RENDER_MODE_PARTIAL` |
| *(double buf)* | `LV_DISPLAY_RENDER_MODE_PARTIAL` (double-buffer handled by `set_buffers` with non-NULL second buf) |
| *(full-screen buf)* | `LV_DISPLAY_RENDER_MODE_FULL` |
| *(direct mode)* | `LV_DISPLAY_RENDER_MODE_DIRECT` |

## Refresh

| v8 | v9 |
|----|----|
| `lv_refr_now(NULL)` | `lv_refr_now(lv_display_get_default())` |
| `lv_obj_invalidate(obj)` | `lv_obj_invalidate(obj)` *(unchanged)* |

## Message Box (used for confirm dialogs)

| v8 | v9 |
|----|----|
| `lv_msgbox_create(parent, title, text, btns[], add_close)` | `lv_msgbox_create(parent)` then `lv_msgbox_add_title()`, `lv_msgbox_add_text()`, `lv_msgbox_add_footer_button()` |
| `lv_msgbox_get_active_btn_text(mbox)` | `lv_msgbox_get_active_btn_text(mbox)` *(check v9 docs)* |
| `lv_msgbox_close(mbox)` | `lv_msgbox_close(lv_obj_get_parent(lv_event_get_target(e)))` from inside event handler |
