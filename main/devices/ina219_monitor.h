#ifndef INA219_MONITOR_H
#define INA219_MONITOR_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power monitor state structure
 */
typedef struct {
    float voltage_v;        // Bus voltage (V)
    float current_ma;       // Current (mA) 
    float power_w;          // Power (W)
    float used_wh;          // Accumulated energy used (Wh)
    float remaining_wh;     // Remaining energy (Wh)
    float soc_pct;          // State of charge percentage
    uint8_t level;          // Battery level: 0=UNKNOWN, 1=LOW, 2=MEDIUM, 3=HIGH
    bool sensor_ok;         // Sensor status
} ina219_power_data_t;

/**
 * @brief Initialize INA219 power monitor
 * @param i2c_port I2C port number
 * @param i2c_addr I2C address (default 0x40, 0x41, 0x44, 0x45)
 * @return ESP_OK on success
 */
esp_err_t ina219_monitor_init(int i2c_port, uint8_t i2c_addr);

/**
 * @brief Deinitialize INA219 power monitor
 * @return ESP_OK on success
 */
esp_err_t ina219_monitor_deinit(void);

/**
 * @brief Read power data from INA219
 * @param power_data Pointer to store power data
 * @return ESP_OK on success
 */
esp_err_t ina219_monitor_read_power_data(ina219_power_data_t *power_data);

/**
 * @brief Set full usable capacity (Wh)
 * @param full_wh Full usable capacity in watt-hours
 * @return ESP_OK on success
 */
esp_err_t ina219_monitor_set_full_capacity(float full_wh);

/**
 * @brief Mark powerbank as fully charged (reset counters)
 * @return ESP_OK on success
 */
esp_err_t ina219_monitor_mark_full(void);

/**
 * @brief Get current status
 * @param power_data Pointer to store current power data
 * @return ESP_OK on success
 */
esp_err_t ina219_monitor_get_status(ina219_power_data_t *power_data);

#ifdef __cplusplus
}
#endif

#endif // INA219_MONITOR_H