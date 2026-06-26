#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Battery level status color
 */
typedef enum {
    BATTERY_COLOR_GREEN,  // >= 5.0V
    BATTERY_COLOR_YELLOW, // 3.7V - 5.0V
    BATTERY_COLOR_RED,    // <= 3.0V
} battery_color_t;

/**
 * @brief Initialize battery monitoring service
 * @return ESP_OK on success
 */
esp_err_t battery_service_init(void);

/**
 * @brief Get current battery voltage in millivolts
 * @param out_voltage_mv Pointer to store voltage (in mV)
 * @return ESP_OK on success
 */
esp_err_t battery_service_get_voltage_mv(int *out_voltage_mv);

/**
 * @brief Get battery color based on current voltage
 * @param out_color Pointer to store color enum
 * @return ESP_OK on success
 */
esp_err_t battery_service_get_color(battery_color_t *out_color);

/**
 * @brief Get LVGL color for battery status
 * @param out_lv_color Pointer to store LVGL color
 * @return ESP_OK on success
 */
esp_err_t battery_service_get_lv_color(lv_color_t *out_lv_color);

/**
 * @brief Deinitialize battery service
 * @return ESP_OK on success
 */
esp_err_t battery_service_deinit(void);

/**
 * @brief Check if INA219 is being used for battery monitoring
 * @return true if INA219 is enabled, false otherwise
 */
bool battery_service_is_ina219_used(void);

#ifdef __cplusplus
}
#endif

#endif // BATTERY_SERVICE_H
