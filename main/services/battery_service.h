#ifndef BATTERY_SERVICE_H
#define BATTERY_SERVICE_H

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Battery level status (based on SOC, not voltage)
 */
typedef enum {
    BATTERY_LEVEL_UNKNOWN = 0,
    BATTERY_LEVEL_LOW,
    BATTERY_LEVEL_MEDIUM,
    BATTERY_LEVEL_HIGH,
} battery_level_t;

/**
 * @brief Initialize battery monitoring service
 * @return ESP_OK on success
 */
esp_err_t battery_service_init(void);

/**
 * @brief Get complete battery power data from INA219
 * @param out_level Pointer to store level enum (HIGH/MEDIUM/LOW/UNKNOWN)
 * @param out_soc_pct Pointer to store state-of-charge percentage (0-100)
 * @param out_remaining_wh Pointer to store remaining energy in watt-hours
 * @return ESP_OK on success
 */
esp_err_t battery_service_get_power_data(battery_level_t *out_level,
                                          float *out_soc_pct,
                                          float *out_remaining_wh);

/**
 * @brief Get LVGL color for battery status (SOC-based)
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
