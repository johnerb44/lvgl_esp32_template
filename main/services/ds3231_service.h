/**
 * @file ds3231_service.h
 * @brief DS3231 RTC service wrapper (feature-flag safe)
 *
 * Thin wrapper around ds3231_rtc that returns placeholder data
 * when CONFIG_LOCKBOX_FEATURE_DS3231 is disabled.
 */

#ifndef DS3231_SERVICE_H
#define DS3231_SERVICE_H

#include "esp_err.h"
#include <stdbool.h>
#include "devices/ds3231_rtc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize DS3231 RTC service
 * @param i2c_port I2C port number
 * @param i2c_addr I2C address (0x68)
 * @return ESP_OK on success (always returns ESP_OK when feature disabled)
 */
esp_err_t ds3231_service_init(int i2c_port, uint8_t i2c_addr);

/**
 * @brief Deinitialize DS3231 RTC service
 * @return ESP_OK always
 */
esp_err_t ds3231_service_deinit(void);

/**
 * @brief Get current date and time
 * @param dt Output datetime
 * @return ESP_OK on success (placeholder when disabled)
 */
esp_err_t ds3231_service_get_datetime(ds3231_datetime_t *dt);

/**
 * @brief Set date and time
 * @param dt Datetime to set
 * @return ESP_OK on success (no-op when disabled)
 */
esp_err_t ds3231_service_set_datetime(const ds3231_datetime_t *dt);

/**
 * @brief Get date only
 * @param d Output date
 * @return ESP_OK on success (placeholder when disabled)
 */
esp_err_t ds3231_service_get_date(ds3231_date_t *d);

/**
 * @brief Set date only
 * @param d Date to set
 * @return ESP_OK on success (no-op when disabled)
 */
esp_err_t ds3231_service_set_date(const ds3231_date_t *d);

/**
 * @brief Set RTC date via separate day/month/year values (admin UI convenience)
 * @param day 1-31
 * @param month 1-12
 * @param year 2000-2099
 * @return ESP_OK on success
 */
esp_err_t ds3231_service_set_date_only(uint8_t day, uint8_t month, uint16_t year);

/**
 * @brief Get service status (initialized or not)
 * @return true if initialized
 */
bool ds3231_service_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* DS3231_SERVICE_H */
