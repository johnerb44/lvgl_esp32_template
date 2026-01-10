# ESP-IDF LVGL Porting Project Instructions

## Project Overview
ESP32-S3 embedded application with LVGL GUI library for Waveshare RGB LCD (ST7701 controller, GT911 touch). This is a security box demo with PIN entry, fingerprint, and facial recognition screens.

## Architecture & Component Structure

### Three-Layer Architecture
1. **Hardware Abstraction** ([waveshare_rgb_lcd_port.c](main/waveshare_rgb_lcd_port.c), [waveshare_rgb_lcd_port.h](main/waveshare_rgb_lcd_port.h))
   - RGB LCD initialization with ST7701 controller (800x480, 16-bit parallel RGB interface)
   - GT911 touch controller I2C setup (I2C_MASTER_NUM 0, pins 8/9)
   - GPIO pin definitions for 16-bit RGB data bus (DATA0-DATA15) and control signals (VSYNC, HSYNC, DE, PCLK)
   - Hardware-specific functions: `waveshare_esp32_s3_rgb_lcd_init()`, `waveshare_esp32_s3_touch_reset()`, `wavesahre_rgb_lcd_bl_on/off()`

2. **LVGL Port Layer** ([lvgl_port.c](main/lvgl_port.c), [lvgl_port.h](main/lvgl_port.h))
   - Thread-safe LVGL integration using FreeRTOS mutex (`lvgl_mux`)
   - Anti-tearing modes (1/2/3) with frame buffer rotation support (0/90/180/270 degrees)
   - VSYNC callback synchronization via `rgb_lcd_on_vsync_event()` and `lvgl_port_notify_rgb_vsync()`
   - Display resolution: 800x480 (LVGL_PORT_H_RES × LVGL_PORT_V_RES)
   - LVGL task management with configurable priority/core/stack via Kconfig

3. **Application Layer** ([main.c](main/main.c))
   - Multi-screen UI state machine (enum `screen_t`: MAIN_MENU, PIN_ENTRY, FINGERPRINT, FACIAL_RECOGNITION)
   - Screen transitions with `lv_scr_load_anim()` (500ms OVER_TOP animation)
   - Entry point: `app_main()` initializes LCD, enables backlight, creates initial screen with mutex locking

### Critical Integration Points
- **VSYNC Synchronization**: Hardware VSYNC events trigger `lvgl_port_notify_rgb_vsync()` to coordinate frame updates and prevent tearing
- **Mutex Protocol**: ALL LVGL API calls (`lv_*`) must be wrapped in `lvgl_port_lock(-1)` / `lvgl_port_unlock()` pairs
- **Component Dependencies**: Managed via [idf_component.yml](main/idf_component.yml): `lvgl/lvgl` (>8.3.9, <9), `esp_lcd_touch_gt911` (^1)

## Build System (ESP-IDF CMake)

### Build Commands
```bash
# Set target (required on first build)
idf.py set-target esp32s3

# Configure project (opens menuconfig TUI)
idf.py menuconfig

# Build only
idf.py build

# Flash and monitor
idf.py -p <PORT> flash monitor

# Full clean rebuild
idf.py fullclean
idf.py build
```

### Configuration System
- **Kconfig**: [main/Kconfig.projbuild](main/Kconfig.projbuild) defines `CONFIG_EXAMPLE_*` parameters
  - `EXAMPLE_LCD_RGB_BOUNCE_BUFFER_HEIGHT`: Bounce buffer size for RGB DMA
  - `EXAMPLE_LVGL_PORT_AVOID_TEAR_ENABLE/MODE`: Anti-tearing strategy (modes 1/2/3)
  - `EXAMPLE_LVGL_PORT_ROTATION_*`: Screen rotation (0/90/180/270)
  - `EXAMPLE_LVGL_PORT_TASK_*`: LVGL task parameters (priority, stack, core, delays)
  - `EXAMPLE_LVGL_PORT_BUF_*`: Buffer allocation (PSRAM vs SRAM)
- **Access Pattern**: Config values consumed via `#include "sdkconfig.h"` → `CONFIG_EXAMPLE_*` macros
- **CMakeLists.txt**: [main/CMakeLists.txt](main/CMakeLists.txt) registers component with sources `waveshare_rgb_lcd_port.c`, `main.c`, `lvgl_port.c`

## Project-Specific Conventions

### ESP-IDF Patterns
- **Logging**: Use `ESP_LOGI(TAG, ...)` with per-file `static const char *TAG = "module_name"`
- **Error Handling**: `ESP_ERROR_CHECK()` for critical operations, explicit `esp_err_t` returns for recoverable errors
- **Entry Point**: `void app_main()` (NOT `int main()`) - called by FreeRTOS after initialization
- **Timing**: Use `vTaskDelay(ms / portTICK_PERIOD_MS)` for delays, NOT `esp_rom_delay_us()` in app code

### LVGL UI Patterns (from [main.c](main/main.c))
- **Screen Creation**: Standalone functions create full screens (`create_*_screen(lv_obj_t *parent)`)
- **Event Handlers**: `static void *_event_cb(lv_event_t *event)` with `lv_event_get_code(event) == LV_EVENT_CLICKED`
- **User Data**: Store button identifiers via `lv_obj_set_user_data(btn, "identifier")`
- **Input Buffer**: Static buffers for PIN entry (`input_buffer[9]`, `input_pos`)

### Hardware-Specific Notes
- **Backlight Control**: Custom backlight on/off functions (not standard ESP-IDF LCD API)
- **Touch Reset Sequence**: GT911 requires I2C write sequence to 0x24/0x38 with specific delays (see `waveshare_esp32_s3_touch_reset()`)
- **RGB Timing**: 16MHz pixel clock, vendor-specific ST7701 initialization sequence in `waveshare_rgb_lcd_port.c`

## Debugging & Monitoring
- **Serial Monitor**: `idf.py -p <PORT> monitor` (Ctrl+] to exit)
- **Common Issues**:
  - Screen tearing → verify AVOID_TEAR_ENABLE in menuconfig and VSYNC callback registration
  - Touch not working → check I2C pull-ups, GT911 reset sequence timing
  - Build errors → ensure IDF_PATH set, correct ESP-IDF version (>=5.0.5)
  - LVGL crashes → verify mutex locking around ALL `lv_*` calls

## When Modifying Code
- **Adding screens**: Follow state machine pattern in `change_screen()`, add enum to `screen_t`, create `create_*_screen()` function
- **Changing resolution**: Update both `LVGL_PORT_H_RES/V_RES` and hardware timings in `waveshare_rgb_lcd_port.c`
- **Adding components**: Declare in [idf_component.yml](main/idf_component.yml), run `idf.py reconfigure` to download
- **Compiler warnings**: `-Wno-attributes` and `-Wno-format` suppression already configured in CMakeLists.txt for LVGL
