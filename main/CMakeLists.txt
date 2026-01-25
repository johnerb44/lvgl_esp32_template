idf_component_register(
    SRCS "waveshare_rgb_lcd_port.c" 
         "main.c" 
         "lvgl_port.c"
         "ui/ui.c"
         "ui/screens/ui_screen_start.c"
         "ui/screens/ui_screen_main.c"
         "ui/screens/ui_screen_biometric.c"
         "ui/screens/ui_screen_fpscan.c"
         "ui/screens/ui_screen_get_pin.c"
         "ui/screens/ui_screen_home.c"
    INCLUDE_DIRS ".")

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
