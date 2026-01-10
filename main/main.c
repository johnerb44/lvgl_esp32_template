#include "waveshare_rgb_lcd_port.h"

// Define APP_TAG for application logging
static const char *APP_TAG = "SECURE_BOX";

// Screen state management
typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_PIN_ENTRY,
    SCREEN_FINGERPRINT,
    SCREEN_FACIAL_RECOGNITION
} screen_t;

static screen_t current_screen = SCREEN_MAIN_MENU;

// Forward declarations of screen creation functions
static void create_main_menu(lv_obj_t *parent);
static void create_pin_entry_screen(lv_obj_t *parent);
static void create_fingerprint_screen(lv_obj_t *parent);
static void create_facial_recognition_screen(lv_obj_t *parent);

// Global variables for our UI elements
static lv_obj_t *display_label;  // Label to show entered digits
static char input_buffer[9];     // Buffer for storing input (8 digits + null terminator)
static uint8_t input_pos = 0;    // Current position in input buffer

// Function to handle smooth screen transitions
static void change_screen(screen_t new_screen)
{
    // Create the new screen on top of the current one
    lv_obj_t *new_scr = lv_obj_create(NULL);
    lv_obj_set_size(new_scr, LV_PCT(100), LV_PCT(100));
    
    // Create content on the new screen based on screen type
    switch (new_screen) {
        case SCREEN_MAIN_MENU:
            create_main_menu(new_scr);
            break;
        case SCREEN_PIN_ENTRY:
            create_pin_entry_screen(new_scr);
            break;
        case SCREEN_FINGERPRINT:
            wavesahre_rgb_lcd_bl_off();
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            wavesahre_rgb_lcd_bl_on();
            create_fingerprint_screen(new_scr);
            break;
        case SCREEN_FACIAL_RECOGNITION:
            create_facial_recognition_screen(new_scr);
            break;
    }
    
    // Use fade transition effect
    lv_scr_load_anim(new_scr, LV_SCR_LOAD_ANIM_OVER_TOP, 500, 0, false);
    
    // Update the current screen state
    current_screen = new_screen;
}

// Button event handler for the main menu
static void menu_btn_event_cb(lv_event_t *event)
{
    lv_obj_t *btn = lv_event_get_target(event);
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        const char *btn_text = lv_obj_get_user_data(btn);
        
        // Change screen based on button clicked
        if (strcmp(btn_text, "PIN") == 0) {
            change_screen(SCREEN_PIN_ENTRY);
        }
        else if (strcmp(btn_text, "Fingerprint") == 0) {
            change_screen(SCREEN_FINGERPRINT);
        }
        else if (strcmp(btn_text, "Facial Recognition") == 0) {
            change_screen(SCREEN_FACIAL_RECOGNITION);
        }
        else if (strcmp(btn_text, "Back") == 0) {
            change_screen(SCREEN_MAIN_MENU);
        }
    }
}

// Button event handler for PIN keypad
static void pin_keypad_event_cb(lv_event_t *event)
{
    lv_obj_t *btn = lv_event_get_target(event);
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        const char *txt = lv_btnmatrix_get_btn_text(btn, lv_btnmatrix_get_selected_btn(btn));
        
        // Handle clear button
        if (strcmp(txt, "Clear") == 0) {
            input_pos = 0;
            input_buffer[0] = '\0';
        } 
        // Handle back button
        else if (strcmp(txt, "Back") == 0) {
            change_screen(SCREEN_MAIN_MENU);
        }
        // Handle digit buttons
        else if (input_pos < 8) {  // Limit to 8 digits
            input_buffer[input_pos] = txt[0];
            input_pos++;
            input_buffer[input_pos] = '\0';
        }
        
        // Update display
        lv_label_set_text(display_label, input_buffer);
    }
}// Function to create the PIN entry screen
static void create_pin_entry_screen(lv_obj_t *parent)
{
    // Clear input buffer
    input_pos = 0;
    input_buffer[0] = '\0';
    lv_color_t cxb = lv_color_hex(0xf5f5f5);

    
    // Create main container
    lv_obj_t *main_cont = lv_obj_create(parent);
    lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_layout(main_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(main_cont, 10, 0);
    
    // Create display box
    lv_obj_t *display_box = lv_obj_create(main_cont);
    lv_obj_set_size(display_box, lv_pct(90), 60);
    lv_obj_set_style_border_width(display_box, 2, 0);
    lv_obj_set_style_border_color(display_box, lv_color_black(), 0);
    lv_obj_set_style_bg_color(display_box, lv_color_white(), 0);
    //lv_obj_set_style_bg_color(display_box, cxb , 0);
    lv_obj_set_style_radius(display_box, 5, 0); 

     // Create display label inside box
    display_label = lv_label_create(display_box);
    lv_obj_center(display_label);
    lv_obj_set_style_text_align(display_label, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_style_text_font(display_label, &lv_font_montserrat_28, 0);
    lv_label_set_text(display_label, "");

    // Create keypad matrix
    static const char * keypad_map[] = {"1", "2", "3", "\n",
                                       "4", "5", "6", "\n",
                                       "7", "8", "9", "\n",
                                       "Back", "0", "Clear", ""};
    
    lv_obj_t *keypad = lv_btnmatrix_create(main_cont);
    lv_obj_set_size(keypad, lv_pct(90), lv_pct(70));
    lv_obj_add_event_cb(keypad, pin_keypad_event_cb, LV_EVENT_CLICKED, NULL);
    lv_btnmatrix_set_map(keypad, keypad_map);

     // Style the Clear button differently
    lv_btnmatrix_set_btn_ctrl(keypad, 11, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_btn_width(keypad, 11, 1);  // Make "Clear" button normal width
    
    // Style the Back button differently
    lv_btnmatrix_set_btn_ctrl(keypad, 9, LV_BTNMATRIX_CTRL_CHECKABLE);
    lv_btnmatrix_set_btn_width(keypad, 9, 1);  // Make "Back" button normal width
    
    lv_obj_set_style_bg_color(keypad, lv_color_hex(0xf5f5f5), 0); 
    
    // Set button styles
    lv_obj_set_style_bg_color(keypad, lv_color_hex(0xc5f5f5), LV_PART_ITEMS);
    lv_obj_set_style_border_width(keypad, 1, LV_PART_ITEMS);
    lv_obj_set_style_border_color(keypad, lv_color_hex(0xdedede), LV_PART_ITEMS);
    lv_obj_set_style_text_font(keypad, &lv_font_montserrat_22, 0);
    
       // Set active button styles
    lv_obj_set_style_bg_color(keypad, lv_color_hex(0x2196f3), LV_PART_ITEMS | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(keypad, LV_OPA_50, LV_PART_ITEMS | LV_STATE_PRESSED);
}

// Function to create the main menu screen
static void create_main_menu(lv_obj_t *parent)
{
    // Create main container
    lv_obj_t *main_cont = lv_obj_create(parent);
    lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_layout(main_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(main_cont, 10, 0);
    
    // Create title label
    lv_obj_t *title_label = lv_label_create(main_cont);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_28, 0);
    lv_label_set_text(title_label, "Secure Box Access");
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_bottom(title_label, 20, 0);
    
    // Create subtitle
    lv_obj_t *subtitle = lv_label_create(main_cont);
    lv_label_set_text(subtitle, "Select Access Method:");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_20, 0);
    lv_obj_set_style_pad_bottom(subtitle, 30, 0);
    
    // Create buttons container
    lv_obj_t *btn_cont = lv_obj_create(main_cont);
    lv_obj_set_size(btn_cont, lv_pct(90), lv_pct(50));
    lv_obj_set_layout(btn_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_obj_set_style_bg_opa(btn_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(btn_cont, 0, 0);
    
    // Create PIN button
    lv_obj_t *pin_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(pin_btn, lv_pct(90), 60);
    lv_obj_set_style_radius(pin_btn, 10, 0);
    lv_obj_set_style_bg_color(pin_btn, lv_color_hex(0x2196f3), 0);
    lv_obj_set_user_data(pin_btn, "PIN");
    lv_obj_add_event_cb(pin_btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *pin_label = lv_label_create(pin_btn);
    lv_label_set_text(pin_label, "PIN Code");
    lv_obj_center(pin_label);
    lv_obj_set_style_text_font(pin_label, &lv_font_montserrat_20, 0);
    
    // Create Fingerprint button
    lv_obj_t *fp_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(fp_btn, lv_pct(90), 60);
    lv_obj_set_style_radius(fp_btn, 10, 0);
    lv_obj_set_style_bg_color(fp_btn, lv_color_hex(0x4caf50), 0);
    lv_obj_set_user_data(fp_btn, "Fingerprint");
    lv_obj_add_event_cb(fp_btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *fp_label = lv_label_create(fp_btn);
    lv_label_set_text(fp_label, "Fingerprint");
    lv_obj_center(fp_label);
    lv_obj_set_style_text_font(fp_label, &lv_font_montserrat_20, 0);
    
    // Create Facial Recognition button
    lv_obj_t *face_btn = lv_btn_create(btn_cont);
    lv_obj_set_size(face_btn, lv_pct(90), 60);
    lv_obj_set_style_radius(face_btn, 10, 0);
    lv_obj_set_style_bg_color(face_btn, lv_color_hex(0xff9800), 0);
    lv_obj_set_user_data(face_btn, "Facial Recognition");
    lv_obj_add_event_cb(face_btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *face_label = lv_label_create(face_btn);
    lv_label_set_text(face_label, "Facial Recognition");
    lv_obj_center(face_label);
    lv_obj_set_style_text_font(face_label, &lv_font_montserrat_20, 0);
}

// Function to create the fingerprint screen
static void create_fingerprint_screen(lv_obj_t *parent)
{
    // Create main container
    lv_obj_t *main_cont = lv_obj_create(parent);
    lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_layout(main_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(main_cont, 10, 0);
    
    // Create title
    lv_obj_t *title = lv_label_create(main_cont);
    lv_label_set_text(title, "Fingerprint Access");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_bottom(title, 30, 0);
    
    // Create instruction
    lv_obj_t *instruction = lv_label_create(main_cont);
    lv_label_set_text(instruction, "Place your finger on the scanner");
    lv_obj_set_style_text_font(instruction, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(instruction, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_bottom(instruction, 50, 0);
    
    // Create fingerprint icon placeholder
    lv_obj_t *icon_area = lv_obj_create(main_cont);
    lv_obj_set_size(icon_area, 150, 150);
    lv_obj_set_style_radius(icon_area, 75, 0);
    lv_obj_set_style_bg_color(icon_area, lv_color_hex(0xeeeeee), 0);
    lv_obj_set_style_border_width(icon_area, 2, 0);
    lv_obj_set_style_border_color(icon_area, lv_color_hex(0x4caf50), 0);
    
    // Create icon label
    lv_obj_t *icon_label = lv_label_create(icon_area);
    lv_label_set_text(icon_label, "👆");
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_48, 0);
    lv_obj_center(icon_label);
    
    // Create back button
    lv_obj_t *back_btn = lv_btn_create(main_cont);
    lv_obj_set_size(back_btn, 120, 50);
    lv_obj_set_style_radius(back_btn, 10, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x3196f3), 0);
    lv_obj_set_style_pad_top(back_btn, 10, 0);
    lv_obj_set_user_data(back_btn, "Back");
    lv_obj_add_event_cb(back_btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_set_style_text_align(back_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Add event callback for back button
    //lv_obj_add_event_cb(back_btn, menu_btn_event_cb, LV_EVENT_CLICKED, "Back");
}

// Function to create the facial recognition screen
static void create_facial_recognition_screen(lv_obj_t *parent)
{
    // Create main container
    lv_obj_t *main_cont = lv_obj_create(parent);
    lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_layout(main_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(main_cont, 10, 0);
    
    // Create title
    lv_obj_t *title = lv_label_create(main_cont);
    lv_label_set_text(title, "Facial Recognition");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_bottom(title, 30, 0);
    
    // Create instruction
    lv_obj_t *instruction = lv_label_create(main_cont);
    lv_label_set_text(instruction, "Look at the camera");
    lv_obj_set_style_text_font(instruction, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(instruction, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_bottom(instruction, 50, 0);
    
    // Create camera view placeholder
    lv_obj_t *camera_area = lv_obj_create(main_cont);
    lv_obj_set_size(camera_area, 200, 160);
    lv_obj_set_style_radius(camera_area, 10, 0);
    lv_obj_set_style_bg_color(camera_area, lv_color_hex(0x333333), 0);
    lv_obj_set_style_border_width(camera_area, 2, 0);
    lv_obj_set_style_border_color(camera_area, lv_color_hex(0xff9800), 0);
    
    // Create icon label
    lv_obj_t *icon_label = lv_label_create(camera_area);
    lv_label_set_text(icon_label, "📷");
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_48, 0);
    lv_obj_center(icon_label);
    
    // Create back button
    lv_obj_t *back_btn = lv_btn_create(main_cont);
    lv_obj_set_size(back_btn, 120, 50);
    lv_obj_set_style_radius(back_btn, 10, 0);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x2196f3), 0);
    lv_obj_set_style_pad_top(back_btn, 10, 0);
    lv_obj_set_user_data(back_btn, "Back");
    lv_obj_add_event_cb(back_btn, menu_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_set_style_text_align(back_label, LV_TEXT_ALIGN_CENTER, 0);
    
    // Add event callback for back button
    //lv_obj_add_event_cb(back_btn, menu_btn_event_cb, LV_EVENT_CLICKED, "Back");
}



void app_main()
{
    waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB LCD 
    wavesahre_rgb_lcd_bl_on();  //Turn on the screen backlight 
    //wavesahre_rgb_lcd_bl_off(); //Turn off the screen backlight 
    
    ESP_LOGI(APP_TAG, "Display menu UI");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(-1)) {
        // Create our main menu UI using the new transition system
        lv_obj_t *initial_scr = lv_obj_create(NULL);
        lv_obj_set_size(initial_scr, LV_PCT(100), LV_PCT(100));
        create_main_menu(initial_scr);
        lv_scr_load(initial_scr);
        current_screen = SCREEN_MAIN_MENU;
        
        // Release the mutex
        lvgl_port_unlock();
    }
}
