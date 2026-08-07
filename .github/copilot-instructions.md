# Secure Lockbox — ESP32-S3 Copilot Instructions

## Project Overview
ESP32-S3 embedded C application: a biometric security lockbox with LVGL GUI on a Waveshare 800×480 RGB touch LCD (ST7701 + GT911). Authentication via fingerprint (R503), face (HLK-TX510), or PIN. Physical lock driven by servo (PCA9685). Biometric modules communicate over SC16IS752 I2C-to-dual-UART bridge. User data persisted as JSON on SD card.

## Build Commands

```bash
idf.py set-target esp32s3          # required once per clean environment
idf.py menuconfig                  # configure features (see Kconfig below)
idf.py build
idf.py -p <PORT> flash monitor     # Ctrl+] to exit monitor
idf.py fullclean && idf.py build   # clean rebuild
```

ESP-IDF ≥ 5.0.5 required. `IDF_PATH` must be set in the environment.

## Architecture

The codebase has five layers; each layer only calls downward:

```
main.c  ──►  app/lockbox_app.c
                 │
                 ├──► services/  (auth, fingerprint, face, lock, session, status)
                 │        │
                 │        ├──► devices/  (r503_device, hlk_tx510_device, pca9685_device, status_inputs)
                 │        │        │
                 │        │        └──► comm/sc16is752_transport  (I2C→dual-UART bridge)
                 │        │
                 │        └──► user_store / user_service  (JSON on SD card via sd/sd_card.c + cJSON)
                 │
                 └──► ui/  (ui.c + ui/screens/ui_screen_*.c)
                          │
                          └──► lvgl_port.c / waveshare_rgb_lcd_port.c  (HAL)
```

### Key modules

| Path | Role |
|------|------|
| `main/app/lockbox_app.c` | Top-level init: calls each service's `_init()` guarded by feature flags |
| `main/services/auth_service.c` | Authentication orchestration, failed-attempt lockout (`LOCKBOX_MAX_FAILED_ATTEMPTS=10`, 60 s) |
| `main/services/session_service.c` | In-memory session: currently-authenticated user; cleared on lock |
| `main/services/fingerprint_service.c` | Wraps R503 device; maps template IDs ↔ `user_t.fingerid` |
| `main/devices/r503_device.c` | R503 UART protocol (AutoIdentify 0x32, AutoEnroll 0x31, ReadIndexTable 0x1F) over SC16IS752 channel 0 |
| `main/devices/hlk_tx510_device.c` | HLK-TX510 face module over SC16IS752 channel 1 |
| `main/comm/sc16is752_transport.c` | SC16IS752 I2C-to-UART bridge; channels: 0=fingerprint, 1=face, 2=status |
| `main/user_store.c` | Load/save `user_t[]` as JSON (`/sdcard/users.jsn`). Atomic write via temp file + rename |
| `main/user_service.c` | Business rules: validate PIN (4–8 digits), unique username, protect last admin |
| `main/ui/ui.c` | Creates all screens at startup; exports `lv_obj_t *ui_screen_*` globals |
| `main/ui/screens/ui_screen_*.c` | One file per screen; each owns its LVGL objects as `static` file-locals |
| `main/user_mgmt_ui.c` | Admin screen: user list, add/edit/delete, biometric status display |
| `main/lvgl_port.c` | FreeRTOS mutex (`lvgl_mux`), VSYNC sync, anti-tear modes |
| `main/waveshare_rgb_lcd_port.c` | ST7701 init sequence, GT911 touch reset, GPIO/RGB timing |

### Battery Monitoring

**Important:** Battery monitoring now uses **only INA219-based monitoring**. All ADC-based battery monitoring code has been removed to simplify the system. The INA219 sensor provides more accurate and comprehensive power monitoring including voltage, current, and power measurements.

### User data model (`user_store.h`)
```c
typedef struct {
    int  userid;      // auto-assigned
    int  fingerid;    // R503 template ID, -1 if none
    int  faceid;      // HLK face ID, -1 if none
    char pin[9];      // 4–8 digit string
    char username[33];
    bool admin;
    char last_logon[33]; // ISO 8601 or ""
} user_t;
```
Stored at `/sdcard/users.jsn`. If file is missing/corrupt, a default admin user is created.

## Feature Flags (Kconfig → `sdkconfig.h`)

All hardware features default to **disabled** (`n`). Enable in `menuconfig` → *Secure Lockbox Integration*:

| Config symbol | What it gates |
|---|---|
| `CONFIG_LOCKBOX_FEATURE_SC16IS752` | SC16IS752 transport (prerequisite for biometrics) |
| `CONFIG_LOCKBOX_FEATURE_R503` | R503 fingerprint sensor (requires SC16IS752) |
| `CONFIG_LOCKBOX_FEATURE_HLK_TX510` | HLK-TX510 face module (requires SC16IS752) |
| `CONFIG_LOCKBOX_FEATURE_PCA9685` | PCA9685 servo driver |
| `CONFIG_LOCKBOX_FEATURE_STATUS_INPUTS` | Lockbox door/bolt status GPIO inputs |
| `CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES` | Mock mode for all hardware (default `y`) |

Guard code with `#if CONFIG_LOCKBOX_FEATURE_*` — see `lockbox_app.c` for the pattern.

## Conventions

### ESP-IDF patterns
- Entry point is `void app_main()`, not `int main()`
- Logging: `static const char *TAG = "MODULE_NAME"` per file; use `ESP_LOGI/W/E`
- Error handling: `ESP_ERROR_CHECK()` for fatal init paths; explicit `esp_err_t` return + `ESP_LOGW` for recoverable errors
- Delays: `vTaskDelay(pdMS_TO_TICKS(ms))`, not `esp_rom_delay_us()` in application code

### LVGL patterns (v9.3)
This project uses **LVGL 9.3.0**. Key v8→v9 API differences to be aware of:
- `lv_display_t` replaces `lv_disp_t`; display handle is now explicit, not global
- Screen management: `lv_screen_load(scr)` / `lv_screen_active()` (replaces `lv_disp_load_scr` / `lv_disp_get_scr_act`)
- Toast/overlay layer: `lv_layer_top()` returns the top overlay layer (use for non-blocking toasts)
- Style selectors: third argument to `lv_obj_set_style_*()` is a part+state selector; use `0` for default (`LV_PART_MAIN | LV_STATE_DEFAULT`)
- **Mutex rule**: every `lv_*` call outside the LVGL task must be wrapped in `lvgl_port_lock(-1)` / `lvgl_port_unlock()`
- **Scrollable containers**: `lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE)` on every container that must not scroll — LVGL 9 enables scrolling by default and touch events from popups will cause jitter otherwise
- Screen objects are `static lv_obj_t *s_*` file-locals within each `ui_screen_*.c`; public screens exported via `ui.h` globals
- Async operations from screens (fingerprint scan, face scan) spawn a FreeRTOS task, then use an `lv_timer` (one-shot, `repeat_count=1`) to post results back to the UI thread — **never call `lv_*` directly from a background task**

### Toast notifications
Reusable pattern (used in `ui_screen_home.c`, `ui_screen_change_pin.c`, `ui_screen_get_pin.c`):
```c
lv_obj_t *toast = lv_obj_create(lv_layer_top());
lv_obj_add_flag(toast, LV_OBJ_FLAG_FLOATING);
lv_obj_clear_flag(toast, LV_OBJ_FLAG_CLICKABLE);
// position/style toast, add label child
lv_timer_t *t = lv_timer_create(cb_delete_toast, 2000, toast);
lv_timer_set_repeat_count(t, 1);
```
The timer callback deletes the toast object. Always create on `lv_layer_top()`, not on the screen, so it survives screen transitions.

### UI screen conventions
- Each screen lives in `ui/screens/ui_screen_<name>.c/.h`
- Public interface: `void ui_screen_<name>_create(void)` + `lv_obj_t *ui_screen_<name>_get(void)`
- Register new screens in `ui/ui.c` (`ui_init`) and add the global to `ui.h`
- Screen-local tag: `static const char *SCREEN_TAG = "UI_SCREEN_NAME"`

### User management
- `user_service_*` enforces all business rules (never bypass with direct `user_store_*` writes from UI)
- `user_service_update()` preserves `fingerid`, `faceid`, and `last_logon` from existing record — do not pass stale zeros
- Deleting/demoting the last admin or the currently-logged-in admin is blocked by `user_service_delete/update()`

## CH422G GPIO Expander

`main/ch422g_driver.c/.h` controls the Waveshare board's I2C GPIO expander (backlight, touch reset, LCD reset, SD card CS, USB select). Key rule: **always bracket SD card operations** with the enable helper:

```c
ch422g_sd_card_enable(I2C_MASTER_NUM, true);   // before mount/read/write
// ... SD operations ...
ch422g_sd_card_enable(I2C_MASTER_NUM, false);   // after
```

The driver is internally mutex-protected. Call `ch422g_init()` once during hardware init before any SD or LCD use.

## Adding a New Screen

1. Create `ui/screens/ui_screen_<name>.c/.h` following the existing pattern
2. Add `void ui_screen_<name>_create(void)` and `lv_obj_t *ui_screen_<name>_get(void)` to `ui.h`
3. Call `ui_screen_<name>_create()` inside `ui_init()` in `ui/ui.c`
4. Add the source file to `SRCS` in `main/CMakeLists.txt`

## Adding a New Hardware Feature

1. Implement the device driver in `devices/<name>_device.c/.h` using `sc16is752_transport` for UART comms
2. Wrap it in a service at `services/<name>_service.c/.h`
3. Add a `CONFIG_LOCKBOX_FEATURE_<NAME>` bool in `Kconfig.projbuild` under *Secure Lockbox Integration*
4. Guard init in `lockbox_app.c` with `#if CONFIG_LOCKBOX_FEATURE_<NAME>`
5. Add sources to `main/CMakeLists.txt`

## Common Debugging

- Screen tearing → verify `EXAMPLE_LVGL_PORT_AVOID_TEAR_ENABLE` in menuconfig; check VSYNC callback is registered
- Touch jitter / screen scrolling unexpectedly → add `lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE)` on affected containers
- Fingerprint "not found" immediately (no finger placed) → check serial log for R503 response byte; 0x01 = PACKET_ERROR, likely command byte wrong
- LVGL crash / assert → missing `lvgl_port_lock` around `lv_*` call
- SD card mount fail → check SPI pins, card format (FAT32), path prefix `/sdcard/`
