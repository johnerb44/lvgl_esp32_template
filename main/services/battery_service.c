/**
 * @file battery_service.c
 * @brief Battery monitoring service — uses INA219 only
 */

#include "battery_service.h"
#include "esp_log.h"
#include "services/ina219_service.h"
#include "sdkconfig.h"

static const char *TAG = "BATTERY_SERVICE";

// Voltage thresholds (in millivolts) - for UI purposes only
#define BATTERY_VOLTAGE_MAX_GREEN   5000  // >= 5.0V: green
#define BATTERY_VOLTAGE_MIN_YELLOW  3700  // 3.7V - 5.0V: yellow
#define BATTERY_VOLTAGE_MAX_RED     3000  // <= 3.0V: red

esp_err_t battery_service_init(void)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219
    // Initialize INA219-based monitoring only
    ESP_LOGI(TAG, "Battery service initialized (INA219-based)");
    return ESP_OK;
#else
    ESP_LOGI(TAG, "Battery service disabled (INA219 not enabled)");
    return ESP_OK;
#endif
}

esp_err_t battery_service_get_voltage_mv(int *out_voltage_mv)
{
    if (out_voltage_mv == NULL) {
        ESP_LOGE(TAG, "Invalid argument: out_voltage_mv is NULL");
        return ESP_ERR_INVALID_ARG;
    }

#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219
    // Use INA219-based monitoring
    ina219_power_data_t power_data = {0};
    esp_err_t ret = ina219_service_get_power_data(&power_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read INA219 power data: %s", esp_err_to_name(ret));
        // Return default voltage on error
        *out_voltage_mv = 5000;
        return ret;
    }
    
    // Convert voltage to millivolts (INA219 reports bus voltage in volts)
    *out_voltage_mv = (int)(power_data.voltage_v * 1000.0f);
    return ESP_OK;
#else
    // Return placeholder voltage if disabled
    *out_voltage_mv = 5000;
    return ESP_OK;
#endif
}

esp_err_t battery_service_get_color(battery_color_t *out_color)
{
    if (out_color == NULL) {
        ESP_LOGE(TAG, "Invalid argument: out_color is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    int voltage_mv = 0;
    esp_err_t ret = battery_service_get_voltage_mv(&voltage_mv);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get battery voltage");
        // Return green as safe default
        *out_color = BATTERY_COLOR_GREEN;
        return ret;
    }

    // Map voltage to color based on thresholds
    if (voltage_mv >= BATTERY_VOLTAGE_MAX_GREEN) {
        *out_color = BATTERY_COLOR_GREEN;
    } else if (voltage_mv >= BATTERY_VOLTAGE_MIN_YELLOW) {
        *out_color = BATTERY_COLOR_YELLOW;
    } else {
        *out_color = BATTERY_COLOR_RED;
    }

    ESP_LOGD(TAG, "Battery voltage: %dmV -> color: %d", voltage_mv, *out_color);
    return ESP_OK;
}

esp_err_t battery_service_get_lv_color(lv_color_t *out_lv_color)
{
    if (out_lv_color == NULL) {
        ESP_LOGE(TAG, "Invalid argument: out_lv_color is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    battery_color_t color;
    esp_err_t ret = battery_service_get_color(&color);
    if (ret != ESP_OK) {
        // Return green as safe default on error
        *out_lv_color = lv_color_hex(0x44ff44);
        return ret;
    }

    switch (color) {
        case BATTERY_COLOR_GREEN:
            *out_lv_color = lv_color_hex(0x44ff44);  // Green
            break;
        case BATTERY_COLOR_YELLOW:
            *out_lv_color = lv_color_hex(0xffff44);  // Yellow
            break;
        case BATTERY_COLOR_RED:
            *out_lv_color = lv_color_hex(0xff4444);  // Red
            break;
        default:
            *out_lv_color = lv_color_hex(0x44ff44);  // Safe default: green
    }

    return ESP_OK;
}

esp_err_t battery_service_deinit(void)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219
    // Deinitialize INA219-based monitoring only
    return ESP_OK;
#else
    return ESP_OK;
#endif
}

bool battery_service_is_ina219_used(void)
{
    // Return true if INA219 feature is enabled
    return CONFIG_LOCKBOX_FEATURE_INA219;
}
