/**
 * @file ina219_service.c
 * @brief INA219 power monitoring service wrapper
 */

#include "ina219_service.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <stdbool.h>

static const char *TAG = "INA219_SERVICE";

bool is_ina219_initialized = false;

esp_err_t ina219_service_init(int i2c_port, uint8_t i2c_addr)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219
    ESP_LOGI(TAG, "Initializing INA219 service on I2C port %d, address 0x%02X", i2c_port, i2c_addr);
    esp_err_t ret = ina219_monitor_init(i2c_port, i2c_addr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize INA219 monitor: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "INA219 service initialized");
    is_ina219_initialized = true;
    return ESP_OK;
#else
    ESP_LOGI(TAG, "INA219 service disabled (LOCKBOX_INTEGRATION or IN219 not enabled)");
    return ESP_OK;
#endif
}

esp_err_t ina219_service_deinit(void)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219  
    return ina219_monitor_deinit();
#else
    return ESP_OK;
#endif
}

esp_err_t ina219_service_get_power_data(ina219_power_data_t *power_data)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219
    return ina219_monitor_read_power_data(power_data);
#else
    if (power_data) {
        // Return placeholder data
        power_data->voltage_v = 5.0f;
        power_data->current_ma = 0.0f;
        power_data->power_w = 0.0f;
        power_data->used_wh = 0.0f;
        power_data->remaining_wh = 74.0f;
        power_data->soc_pct = 100.0f;
        power_data->level = 3;  // HIGH
        power_data->sensor_ok = false;
    }
    return ESP_OK;
#endif
}

esp_err_t ina219_service_set_full_capacity(float full_wh)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE ||CONFIG_LOCKBOX_FEATURE_INA219
    return ina219_monitor_set_full_capacity(full_wh);
#else
    return ESP_OK;
#endif
}

esp_err_t ina219_service_mark_full(void)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219
    return ina219_monitor_mark_full();
#else
    return ESP_OK;
#endif
}

esp_err_t ina219_service_get_status(ina219_power_data_t *power_data)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE || CONFIG_LOCKBOX_FEATURE_INA219
    return ina219_monitor_get_status(power_data);
#else
    if (power_data) {
        // Return placeholder data
        power_data->voltage_v = 5.0f;
        power_data->current_ma = 0.0f;
        power_data->power_w = 0.0f;
        power_data->used_wh = 0.0f;
        power_data->remaining_wh = 74.0f;
        power_data->soc_pct = 100.0f;
        power_data->level = 3;  // HIGH
        power_data->sensor_ok = false;
    }
    return ESP_OK;
#endif
}