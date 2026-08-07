#ifndef INA219_SERVICE_H
#define INA219_SERVICE_H

#include "esp_err.h"
#include "devices/ina219_monitor.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize INA219 power monitoring service
 * @param i2c_port I2C port number
 * @param i2c_addr I2C address (default 0x40, 0x41, 0x44, 0x45)
 * @return ESP_OK on success
 */
esp_err_t ina219_service_init(int i2c_port, uint8_t i2c_addr);
extern bool is_ina219_initialized;

/**
 * @brief Deinitialize INA219 power monitoring service
 * @return ESP_OK on success
 */
esp_err_t ina219_service_deinit(void);

/**
 * @brief Get current power data 
 * @param power_data Pointer to store power data
 * @return ESP_OK on success
 */
esp_err_t ina219_service_get_power_data(ina219_power_data_t *power_data);

/**
 * @brief Set full usable capacity (Wh)
 * @param full_wh Full usable capacity in watt-hours
 * @return ESP_OK on success
 */
esp_err_t ina219_service_set_full_capacity(float full_wh);

/**
 * @brief Mark powerbank as fully charged (reset counters)  
 * @return ESP_OK on success
 */
esp_err_t ina219_service_mark_full(void);

/**
 * @brief Get current service status
 * @param power_data Pointer to store current data
 * @return ESP_OK on success
 */
esp_err_t ina219_service_get_status(ina219_power_data_t *power_data);

#ifdef __cplusplus
}
#endif

#endif // INA219_SERVICE_H