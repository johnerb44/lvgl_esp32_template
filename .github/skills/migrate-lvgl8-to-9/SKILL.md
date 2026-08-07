---
name: migrate-lvgl8-to-9
description: 'Audit and migrate LVGL v8 API usage to LVGL v9. Use when: porting code from LVGL 8 to LVGL 9, encountering deprecated lv_disp_t, lv_disp_load_scr, lv_disp_get_scr_act, lv_color_hex, lv_font_montserrat, or other v8-only symbols. Covers display, indev, screen, style, and draw API changes.'
argument-hint: 'optional: path to file or folder to audit'
---

# Migrate LVGL v8 → v9

This project uses **LVGL 9.3.0**. This skill audits source files for deprecated v8 APIs and replaces them with v9 equivalents.

## When to Use

- You see compiler errors for unknown `lv_disp_t`, `lv_disp_load_scr`, `lv_disp_get_scr_act`, or similar symbols
- You're porting a new screen or driver from another project that targets LVGL v8
- You want a pre-commit audit of changed UI files

## Procedure

1. **Identify targets**: If a path argument was given, scope to that path; otherwise scan `main/` for `.c` and `.h` files
2. **Grep for v8 symbols** using the [API mapping reference](./references/api-mapping.md)
3. **Replace each hit** with its v9 equivalent — apply the rules in the mapping table precisely
4. **Check style calls**: ensure third argument to `lv_obj_set_style_*()` uses `LV_PART_MAIN | LV_STATE_DEFAULT` (or `0`) not a bare `LV_STATE_*`
5. **Verify mutex coverage**: any new `lv_*` call outside the LVGL task must be wrapped in `lvgl_port_lock(-1)` / `lvgl_port_unlock()`
6. **Build check**: run `idf.py build` (or point to `build_idf_validate/`) to confirm no remaining v8 symbol errors

## Key Rules

- **Never** call `lv_*` directly from a FreeRTOS task — post back via `lv_timer` (one-shot, `repeat_count=1`)
- **Never** use `lv_disp_t *` as a type; use `lv_display_t *`
- Screen globals in `ui.h` are `lv_obj_t *`, not `lv_disp_t *`
- The display handle is obtained from `lv_display_create()` at init time; do not call `lv_disp_get_default()`
