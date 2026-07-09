idf_component_register(
    SRCS "user_mgmt_ui.c" "user_service.c" "user_store.c" "ch422g_driver.c" "waveshare_rgb_lcd_port.c"
         "main.c"
         "lvgl_port.c"
         "comm/sc16is752_transport.c"
         "devices/r503_device.c"
         "devices/hlk_tx510_device.c"
         "devices/pca9685_device.c"
         "devices/status_inputs.c"
 
         "services/auth_service.c"
         "services/fingerprint_service.c"
         "services/face_service.c"
         "services/lock_service.c"
         "services/session_service.c"
         "services/status_service.c"
         "services/battery_service.c"
         "services/sleep_service.c"
         "app/lockbox_app.c"
         "ui/ui.c"
         "ui/screens/ui_screen_start.c"
         "ui/screens/ui_screen_main.c"
         "ui/screens/ui_screen_biometric.c"
         "ui/screens/ui_screen_fpscan.c"
         "ui/screens/ui_screen_facescan.c"
         "ui/screens/ui_screen_get_pin.c"
         "ui/screens/ui_screen_change_pin.c"
         "ui/screens/ui_screen_home.c"
         "ui/screens/ui_screen_enroll.c"
         "ui/screens/ui_screen_fp_manage.c"
         "ui/screens/ui_screen_rtc_date.c"
         "devices/ina219_monitor.c"
         "devices/ds3231_rtc.c"
         "sd/sd_card.c"
         "cJSON/cJSON.c"
         "services/ina219_service.c"
         "services/ds3231_service.c"

    INCLUDE_DIRS "."
                 "comm"
                 "devices"
                 "services"
                 "app"
                 "cJSON"
                 "sd"
                 "ui"
     
     REQUIRES esp_idf_lib_helpers
              esp_adc
              fatfs
              nvs_flash
)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
