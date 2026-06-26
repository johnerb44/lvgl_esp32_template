/**
 * @file battery_service.c
 * @brief Battery monitoring service — SOC-based display via INA219
 */

#include "battery_service.h"
#include "services/ina219_service.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "BATTERY_SERVICE";

esp_err_t battery_service_init(void)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219
    ESP_LOGI(TAG, "Battery service initialized (INA219-based SOC monitoring)");
    return ESP_OK;
#else
    ESP_LOGI(TAG, "Battery service disabled (INA219 not enabled)");
    return ESP_OK;
#endif
}

esp_err_t battery_service_get_power_data(battery_level_t *out_level,
                                          float *out_soc_pct,
                                          float *out_remaining_wh)
{
    ina219_power_data_t power_data = {0};
    esp_err_t ret = ina219_service_get_power_data(&power_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read INA219 power data: %s", esp_err_to_name(ret));
        // Return UNKNOWN state on error
        if (out_level)  *out_level = BATTERY_LEVEL_UNKNOWN;
        if (out_soc_pct) *out_soc_pct = 0.0f;
        if (out_remaining_wh) *out_remaining_wh = 0.0f;
        return ret;
    }

    // Map INA219 level (0=UNKNOWN, 1=LOW, 2=MEDIUM, 3=HIGH) to battery_level_t
    if (out_level) {
        *out_level = (battery_level_t)power_data.level;
    }
    if (out_soc_pct) {
        *out_soc_pct = power_data.soc_pct;
    }
    if (out_remaining_wh) {
        *out_remaining_wh = power_data.remaining_wh;
    }

    ESP_LOGD(TAG, "Battery: SOC=%.1f%% level=%u remaining=%.1fWh sensor_ok=%d",
             power_data.soc_pct, power_data.level, power_data.remaining_wh,
             power_data.sensor_ok);
    return ESP_OK;
}

esp_err_t battery_service_get_lv_color(lv_color_t *out_lv_color)
{
    if (out_lv_color == NULL) {
        ESP_LOGE(TAG, "Invalid argument: out_lv_color is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    battery_level_t level;
    esp_err_t ret = battery_service_get_power_data(&level, NULL, NULL);
    if (ret != ESP_OK) {
        *out_lv_color = lv_color_hex(0x888888);  // Gray = sensor unknown
        return ret;
    }

    switch (level) {
        case BATTERY_LEVEL_HIGH:
            *out_lv_color = lv_color_hex(0x44ff44);  // Green
            break;
        case BATTERY_LEVEL_MEDIUM:
            *out_lv_color = lv_color_hex(0xffff44);  // Yellow
            break;
        case BATTERY_LEVEL_LOW:
            *out_lv_color = lv_color_hex(0xff4444);  // Red
            break;
        default:  // UNKNOWN
            *out_lv_color = lv_color_hex(0x888888);  // Gray
            break;
    }

    return ESP_OK;
}

esp_err_t battery_service_deinit(void)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219
    return ESP_OK;
#else
    return ESP_OK;
#endif
}

bool battery_service_is_ina219_used(void)
{
    return CONFIG_LOCKBOX_FEATURE_INA219;
}
