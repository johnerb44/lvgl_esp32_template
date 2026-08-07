/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Bus Mutual Exclusion Layer
 *
 * Provides a global mutex to serialize access to the I2C bus (I2C_NUM_0) across
 * all devices (DS3231 RTC, CH422G GPIO expander, SC16IS752, INA219, GT911).
 * This prevents I2C collisions and race conditions during concurrent operations.
 *
 * Usage:
 *   i2c_bus_lock("DS3231_get_date");    // Acquire lock
 *   // ... perform I2C operations ...
 *   i2c_bus_unlock("DS3231_get_date");  // Release lock
 */

#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C bus lock timeout in milliseconds (generous for safety).
 *        If a lock cannot be acquired within this time, a deadlock is logged.
 */
#define I2C_BUS_LOCK_TIMEOUT_MS 3000

/**
 * @brief Initialize the global I2C bus mutex.
 *
 * Must be called once during initialization, before any I2C device accesses
 * the bus (typically in app_main() after LCD init but before SD card or RTC).
 *
 * Creates a recursive mutex to allow the same task to acquire the lock multiple
 * times (for nested lock scenarios, such as user_service calling user_store_load).
 *
 * @return ESP_OK if successful, ESP_FAIL if mutex creation fails.
 */
esp_err_t i2c_bus_init(void);

/**
 * @brief Acquire the I2C bus lock (recursive).
 *
 * Blocks until the lock is acquired or timeout occurs.
 * Can be called multiple times by the same task (recursive acquire).
 * Logs lock acquisition with the owner name for debugging.
 *
 * @param owner Human-readable identifier of the caller (e.g., "DS3231_get_date", "USER_STORE_load").
 *              Used in log messages to track lock contention.
 */
void i2c_bus_lock(const char *owner);

/**
 * @brief Release the I2C bus lock (recursive).
 *
 * Logs lock release with the owner name for debugging.
 * If called multiple times by the same task, must be paired with matching i2c_bus_lock calls.
 *
 * @param owner Human-readable identifier of the caller (must match previous i2c_bus_lock() call).
 */
void i2c_bus_unlock(const char *owner);

#ifdef __cplusplus
}
#endif

#endif // I2C_BUS_H
