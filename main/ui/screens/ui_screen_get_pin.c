/**
 * @file ui_screen_get_pin.c
 * @brief Template source file for LVGL objects
 */

/*********************
 *      INCLUDES
 *********************/

#include "ui_screen_get_pin.h"
#include "../ui.h"
#include "esp_log.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/
#define PIN_MAX_LENGTH 4

/**********************
 *      TYPEDEFS
 **********************/

/***********************
 *  STATIC VARIABLES
 **********************/
static char pin_buffer[PIN_MAX_LENGTH + 1] = {0};  // +1 for null terminator
static uint8_t pin_length = 0;
static lv_obj_t *pin_display_label = NULL;
static lv_obj_t *show_pin_checkbox = NULL;
static bool show_pin = false;  // Track whether to show actual PIN or asterisks
/***********************
 *  STATIC PROTOTYPES
 **********************/
static void update_pin_display(void);
static void button_matrix_event_cb(lv_event_t *event);
static void submit_btn_event_cb(lv_event_t *event);
static void show_pin_checkbox_event_cb(lv_event_t *event);
static void screen_loaded_event_cb(lv_event_t *event);
static const char *SCREEN_TAG = "UI_SCREEN_GET_PIN";

static void toast_timer_cb(lv_timer_t *timer);
static void navigate_to_home_cb(lv_timer_t *timer);

static void create_toast(const char *text, int timeout_ms);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Update the PIN display with current entered digits
 */
static void update_pin_display(void)
{
    if (pin_display_label == NULL) {
        return;
    }
    
    char display_text[PIN_MAX_LENGTH + 1];
    
    if (pin_length == 0) {
        strcpy(display_text, "----");
    } else {
        for (int i = 0; i < PIN_MAX_LENGTH; i++) {
            if (i < pin_length) {
                if (show_pin) {
                    display_text[i] = pin_buffer[i];  // Show actual digit
                } else {
                    display_text[i] = '*';  // Show asterisk for entered digits
                }
            } else {
                display_text[i] = '-';  // Show dash for empty positions
            }
        }
        display_text[PIN_MAX_LENGTH] = '\0';
    }
    
    lv_label_set_text(pin_display_label, display_text);
    ESP_LOGI(SCREEN_TAG, "PIN display updated: %s (length: %d)", display_text, pin_length);
}

/**
 * @brief Handle button matrix keypad events
 */
static void button_matrix_event_cb(lv_event_t *event)
{
    lv_obj_t *btn_matrix = lv_event_get_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    
    if (code == LV_EVENT_VALUE_CHANGED) {
        uint32_t btn_id = lv_buttonmatrix_get_selected_button(btn_matrix);
        const char *btn_text = lv_buttonmatrix_get_button_text(btn_matrix, btn_id);
        
        ESP_LOGI(SCREEN_TAG, "Button pressed: %s", btn_text);
        
        // Handle numeric buttons (0-9)
        if (btn_text[0] >= '0' && btn_text[0] <= '9' && btn_text[1] == '\0') {
            if (pin_length < PIN_MAX_LENGTH) {
                pin_buffer[pin_length] = btn_text[0];
                pin_length++;
                pin_buffer[pin_length] = '\0';
                update_pin_display();
                ESP_LOGI(SCREEN_TAG, "Digit added. Current PIN: %s", pin_buffer);
            } else {
                ESP_LOGW(SCREEN_TAG, "PIN already at max length (%d)", PIN_MAX_LENGTH);
            }
        }
        // Handle Clear button
        else if (strcmp(btn_text, "Clear") == 0) {
            memset(pin_buffer, 0, sizeof(pin_buffer));
            pin_length = 0;
            update_pin_display();
            ESP_LOGI(SCREEN_TAG, "PIN cleared");
        }
        // Handle Backspace button
        else if (strcmp(btn_text, "Backspace") == 0) {
            if (pin_length > 0) {
                pin_length--;
                pin_buffer[pin_length] = '\0';
                update_pin_display();
                ESP_LOGI(SCREEN_TAG, "Digit removed. Current PIN: %s", pin_buffer);
            } else {
                ESP_LOGW(SCREEN_TAG, "PIN is empty, nothing to backspace");
            }
        }
    }
}

/**
 * @brief Handle submit button click event
 */
static void submit_btn_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
        ESP_LOGI(SCREEN_TAG, "Submit button clicked. Entered PIN: %s", pin_buffer);
        char pin_valid[] = "1234"; // Example valid PIN
        
        // TODO: In future, compare pin_buffer with stored PIN here
        // Show toast notification
        if (strcmp(pin_buffer, pin_valid) == 0) {
            create_toast("PIN Accepted", 2000); // Shows "PIN Accepted" for 2 seconds
            // Delay navigation to home screen until after toast is shown (8000ms + 500ms buffer)
            // Clear PIN for next entry
            memset(pin_buffer, 0, sizeof(pin_buffer));
            pin_length = 0;
            lv_timer_create(navigate_to_home_cb, 2500, NULL);
        } 
        else {
            create_toast("Invalid PIN", 2000); // Shows "Invalid PIN" for 2 seconds
            // Clear PIN for next entry
            memset(pin_buffer, 0, sizeof(pin_buffer));
            pin_length = 0;
            update_pin_display();

            //lv_timer_create(navigate_to_home_cb, 5500, NULL);
        }
        
        
        
        // Delay navigation to home screen until after toast is shown (8000ms + 500ms buffer)
        //lv_timer_create(navigate_to_home_cb, 5500, NULL);
    }
}

/**
 * @brief Handle show PIN checkbox events
 */
static void show_pin_checkbox_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *checkbox = lv_event_get_target(event);
    
    if (code == LV_EVENT_VALUE_CHANGED) {
        show_pin = lv_obj_has_state(checkbox, LV_STATE_CHECKED);
        ESP_LOGI(SCREEN_TAG, "Show PIN checkbox changed: %s", show_pin ? "checked" : "unchecked");
        update_pin_display();
    }
}

/**
 * @brief Handle screen loaded event to reset PIN state
 */
static void screen_loaded_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) == LV_EVENT_SCREEN_LOADED) {
        // Reset PIN state when screen is loaded
        memset(pin_buffer, 0, sizeof(pin_buffer));
        pin_length = 0;
        show_pin = false;
        
        // Reset checkbox to unchecked
        if (show_pin_checkbox != NULL) {
            lv_obj_clear_state(show_pin_checkbox, LV_STATE_CHECKED);
        }
        
        // Update the display to show cleared state
        update_pin_display();
        
        ESP_LOGI(SCREEN_TAG, "Screen loaded - PIN state reset");
    }
}

static void create_toast(const char *text, int timeout_ms) {
    // 1. Create a container on the top layer (persists across screen changes)
    lv_obj_t *toast = lv_obj_create(lv_layer_top());
    lv_obj_set_size(toast, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(toast, LV_ALIGN_TOP_LEFT, 0, 20); // Top center with padding
    lv_obj_set_x(toast, 30); 
    lv_obj_set_style_bg_color(toast, lv_color_hex(0xe19419), LV_PART_MAIN);
    lv_obj_set_style_text_font(toast, &lv_font_montserrat_24, 0);
    lv_obj_set_style_radius(toast, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(toast, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(toast, 15, LV_PART_MAIN);
    
    // Make it non-clickable so it doesn't block user interaction
    lv_obj_add_flag(toast, LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_CLICKABLE);

    // 2. Add text
    lv_obj_t *label = lv_label_create(toast);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(label);

    // 3. Create a one-shot timer to delete the toast
    lv_timer_t *timer = lv_timer_create(toast_timer_cb, timeout_ms, toast);
    lv_timer_set_repeat_count(timer, 1);
}


void ui_screen_get_pin_create(void)
{
    LV_TRACE_OBJ_CREATE("begin");

    // Reset PIN state when creating screen
    memset(pin_buffer, 0, sizeof(pin_buffer));
    pin_length = 0;
    show_pin = false;  // Default to hiding PIN
    
    ui_screen_get_pin = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_screen_get_pin, lv_color_hex(0x041d3a), 0);
    lv_obj_set_style_text_color(ui_screen_get_pin, lv_color_hex3(0xfff), 0);
    
    // Add event handler for when screen is loaded
    lv_obj_add_event_cb(ui_screen_get_pin, screen_loaded_event_cb, LV_EVENT_SCREEN_LOADED, NULL);

    lv_obj_t * get_pin_title = lv_label_create(ui_screen_get_pin);
    lv_label_set_text(get_pin_title, "Enter PIN");
    lv_obj_set_style_text_font(get_pin_title, &lv_font_montserrat_28, 0);
    lv_obj_set_align(get_pin_title, LV_ALIGN_TOP_MID);
    lv_obj_set_y(get_pin_title, 10);
    
    pin_display_label = lv_label_create(ui_screen_get_pin);
    lv_obj_set_align(pin_display_label, LV_ALIGN_CENTER);
    lv_obj_set_style_width(pin_display_label, 200, 0);
    lv_obj_set_style_height(pin_display_label, 50, 0);
    lv_obj_set_y(pin_display_label, -140);
    lv_obj_set_style_outline_width(pin_display_label, 5, 0);
    lv_obj_set_style_outline_color(pin_display_label, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_text_color(pin_display_label, lv_color_hex3(0x111), 0);
    lv_obj_set_style_text_align(pin_display_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_bg_color(pin_display_label, lv_color_hex3(0xfff), 0);
    lv_obj_set_style_bg_opa(pin_display_label, 255, 0);
    lv_obj_set_style_pad_top(pin_display_label, 12, 0);
    lv_obj_set_style_text_opa(pin_display_label, 255, 0);
    lv_obj_set_style_text_font(pin_display_label, &lv_font_montserrat_30, 0);
    lv_label_set_text(pin_display_label, "----");
    
    // Create "Show PIN" checkbox to the right of the display
    show_pin_checkbox = lv_checkbox_create(ui_screen_get_pin);
    lv_checkbox_set_text(show_pin_checkbox, "Show PIN");
    lv_obj_set_align(show_pin_checkbox, LV_ALIGN_CENTER);
    lv_obj_set_width(show_pin_checkbox, 120);
    lv_obj_set_height(show_pin_checkbox, 40);
    lv_obj_set_x(show_pin_checkbox, 180);
    lv_obj_set_y(show_pin_checkbox, -140);
    lv_obj_set_style_text_font(show_pin_checkbox, &lv_font_montserrat_16, 0);
    lv_obj_clear_state(show_pin_checkbox, LV_STATE_CHECKED);  // Default to unchecked
    lv_obj_add_event_cb(show_pin_checkbox, show_pin_checkbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    lv_obj_t * button_matrix_pin = lv_buttonmatrix_create(ui_screen_get_pin);
    lv_obj_set_align(button_matrix_pin, LV_ALIGN_LEFT_MID);
    lv_obj_set_style_text_font(button_matrix_pin, &lv_font_montserrat_26, 0);
    lv_obj_set_height(button_matrix_pin, 240);
    lv_obj_set_width(button_matrix_pin, 530);
    lv_obj_set_style_text_align(button_matrix_pin, LV_TEXT_ALIGN_AUTO, 0);
    lv_obj_set_style_text_color(button_matrix_pin, lv_color_hex(0x1a1919), 0);
    lv_obj_set_x(button_matrix_pin, 140);
    lv_obj_set_y(button_matrix_pin, 20);
    static const char *button_matrix_pin_map_0[] = {"1", "2", "3", "\n", "4", "5", "6", "\n", "7", "8", "9", "\n", "Clear", "0", "Backspace", NULL};
    lv_buttonmatrix_set_map(button_matrix_pin, button_matrix_pin_map_0);
    lv_obj_add_event_cb(button_matrix_pin, button_matrix_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    lv_obj_t * submit_button = lv_button_create(ui_screen_get_pin);
    lv_obj_set_align(submit_button, LV_ALIGN_BOTTOM_MID);
    lv_obj_set_y(submit_button, -30);
    lv_obj_set_style_bg_color(submit_button, lv_color_hex(0xe19419), 0);
    
    lv_obj_t * lv_label_0 = lv_label_create(submit_button);
    lv_label_set_text(lv_label_0, "SUBMIT");
    lv_obj_set_style_text_color(lv_label_0, lv_color_hex3(0x000), 0);
    lv_obj_set_style_text_font(lv_label_0, &lv_font_montserrat_26, 0);
    
    lv_obj_add_event_cb(submit_button, submit_btn_event_cb, LV_EVENT_CLICKED, NULL);

    LV_TRACE_OBJ_CREATE("finished");
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

// Timer callback to delete the toast
static void toast_timer_cb(lv_timer_t *timer) {
    lv_obj_t *toast = lv_timer_get_user_data(timer);
    // Optional: Add a fade-out animation here before deletion
    lv_obj_del(toast);
}

// Timer callback to navigate to home screen
static void navigate_to_home_cb(lv_timer_t *timer) {
    (void)timer;  // Unused parameter
    ESP_LOGI(SCREEN_TAG, "Navigating to home screen");
    lv_scr_load_anim(ui_screen_home, LV_SCR_LOAD_ANIM_MOVE_TOP, 500, 0, false);
}
