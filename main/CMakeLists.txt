idf_component_register(
    SRCS "user_mgmt_ui.c" "user_service.c" "user_store.c" "ch422g_driver.c" "waveshare_rgb_lcd_port.c" 
         "main.c" 
         "lvgl_port.c"
         "ui/ui.c"
         "ui/screens/ui_screen_start.c"
         "ui/screens/ui_screen_main.c"
         "ui/screens/ui_screen_biometric.c"
         "ui/screens/ui_screen_fpscan.c"
         "ui/screens/ui_screen_get_pin.c"
         "ui/screens/ui_screen_change_pin.c"
         "ui/screens/ui_screen_home.c"
         "sd/sd_card.c" 
         "cJSON/cJSON.c"

    INCLUDE_DIRS "."
                 "cJSON" 
                 "sd" 
                 "ui"
)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
