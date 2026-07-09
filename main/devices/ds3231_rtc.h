/**
 * @file ds321_rtc.h
 * @brief DS3231 RTC I2C driver header
 *
 * Provides get/set date and time functions for the DS3231 RTC
 * (onboard coin cell, address 0x68) on the shared I2C bus.
 * All time/date values use BCD encoding.
 */

#ifndef DS3231_RTC_H
#define DS3231_RTC_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DS3231 I2C address */
#define DS3231_I2C_ADDRESS  0x68

/* DS3231 register addresses */
#define DS3231_REG_sec      0x00
#define DS3231_REG_min      0x01
#define DS3231_REG_hour     0x02
#define DS3231_REG_day      0x03
#define DS3231_REG_date     0x04
#define DS3231_REG_month    0x05
#define DS3231_REG_year     0x06
#define DS3231_REG_control  0x0E
#define DS3231_REG_status   0x0F

/* Month register bit: century bit (bit 7) */
#define DS3231_MONTH_CENTURY_BIT  (1 << 7)

/* Time structure */
typedef struct {
    uint8_t seconds;  // 0-59
    uint8_t minutes;  // 0-59
    uint8_t hour;     // 0-23 (24h mode)
} ds3231_time_t;

/* Date structure */
typedef struct {
    uint16_t year;    // 2000-2099
    uint8_t  month;   // 1-12
    uint8_t  day;     // 1-31
} ds3231_date_t;

/* Combined datetime */
typedef struct {
    ds3231_date_t date;
    ds3231_time_t time;
} ds3231_datetime_t;

/* String date format: dd-mm-yyyy (11 bytes with null) */
#define DS3231_DATE_STR_LEN  11

/*
 * @brief Initialize DS3231 driver (I2C bus must already be up)
 * @param i2c_port I2C port number (must match existing bus)
 * @param i2c_addr I2C address (default 0x68, unused param for API compat)
 * @return ESP_OK on success, ESP_ERR_TIMEOUT if device not found
 */
esp_err_t ds3231_rtc_init(int i2c_port, uint8_t i2c_addr);

/*
 * @brief Deinitialize DS3231 driver
 * @return ESP_OK always
 */
esp_err_t ds3231_rtc_deinit(void);

/*
 * @brief Check if the driver is initialized
 * @return true if initialized
 */
bool ds3231_rtc_is_initialized(void);

/*
 * @brief Get current date and time from DS3231
 * @param dt Output datetime structure
 * @return ESP_OK on success
 */
esp_err_t ds3231_rtc_get_datetime(ds3231_datetime_t *dt);

/*
 * @brief Set date and time on DS3231
 * @param dt Datetime to set
 * @return ESP_OK on success
 */
esp_err_t ds3231_rtc_set_datetime(const ds3231_datetime_t *dt);

/*
 * @brief Get current time only (hours, minutes, seconds)
 * @param t Output time
 * @return ESP_OK on success
 */
esp_err_t ds3231_rtc_get_time(ds3231_time_t *t);

/*
 * @brief Set time only (hours, minutes, seconds)
 * @param t Time to set
 * @return ESP_OK on success
 */
esp_err_t ds3231_rtc_set_time(const ds3231_time_t *t);

/*
 * @brief Get current date only
 * @param d Output date
 * @return ESP_OK on success
 */
esp_err_t ds3231_rtc_get_date(ds3231_date_t *d);

/*
 * @brief Set date only (month, day, year) — preserves time
 * Month register bit 7 = century bit (1 = 2000s)
 * @param d Date to set
 * @return ESP_OK on success
 */
esp_err_t ds3231_rtc_set_date(const ds3231_date_t *d);

/*
 * @brief Format date as "dd-mm-yyyy" string (11 bytes with null terminator)
 * @param d Date to format
 * @param out_buf Output buffer (must be at least DS3231_DATE_STR_LEN bytes)
 */
void ds3231_date_to_string(const ds3231_date_t *d, char *out_buf);

/*
 * @brief Parse "dd-mm-yyyy" string to date
 * @param str Input string (format: dd-mm-yyyy)
 * @param d Output date
 * @return ESP_OK on valid input
 */
esp_err_t ds3231_date_from_string(const char *str, ds3231_date_t *d);

#ifdef __cplusplus
}
#endif

#endif /* DS3231_RTC_H */
