/**
 * @file ds3231_service.c
 * @brief DS3231 RTC service wrapper (feature-flag safe)
 */

#include "ds3231_service.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "DS3231_SERVICE";
static bool s_initialized = false;

/* ========== Feature disabled stubs ========== */

#if !CONFIG_LOCKBOX_FEATURE_DS3231

esp_err_t ds3231_service_init(int i2c_port, uint8_t i2c_addr)
{
    ESP_LOGW(TAG, "DS3231 feature disabled — using default date (01-01-2026)");
    s_initialized = true;
    return ESP_OK;
}

esp_err_t ds3231_service_deinit(void) { s_initialized = false; return ESP_OK; }
bool ds3231_service_is_initialized(void) { return s_initialized; }

esp_err_t ds3231_service_get_datetime(ds3231_datetime_t *dt)
{
    if (!dt) return ESP_ERR_INVALID_ARG;
    memset(&dt->date, 0, sizeof(ds3231_date_t));
    memset(&dt->time, 0, sizeof(ds3231_time_t));
    dt->date = s_default_date;
    return ESP_OK;
}

esp_err_t ds3231_service_set_datetime(const ds3231_datetime_t *dt)
{
    (void)dt;
    return ESP_OK;
}

esp_err_t ds3231_service_get_date(ds3231_date_t *d)
{
    if (!d) return ESP_ERR_INVALID_ARG;
    *d = s_default_date;
    return ESP_OK;
}

esp_err_t ds3231_service_set_date(const ds3231_date_t *d)
{
    (void)d;
    return ESP_OK;
}

esp_err_t ds3231_service_set_date_only(uint8_t day, uint8_t month, uint16_t year)
{
    (void)day; (void)month; (void)year;
    return ESP_OK;
}

#else
/* ========== Feature enabled — proxy to device driver ========== */

esp_err_t ds3231_service_init(int i2c_port, uint8_t i2c_addr)
{
    ESP_LOGI(TAG, "Initializing DS3231 service on I2C port %d, address 0x%02X",
             i2c_port, i2c_addr);

    esp_err_t ret = ds3231_rtc_init(i2c_port, i2c_addr);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "DS3231 hardware not detected: %s — operating in software mode", esp_err_to_name(ret));
    }

    s_initialized = true;
    ESP_LOGI(TAG, "DS3231 service initialized (RTC=%s)",
             (ret == ESP_OK) ? "found" : "not found (software mode)");
    return ESP_OK;
}

esp_err_t ds3231_service_deinit(void)
{
    s_initialized = false;
    return ds3231_rtc_deinit();
}

bool ds3231_service_is_initialized(void)
{
    return s_initialized;
}

esp_err_t ds3231_service_get_datetime(ds3231_datetime_t *dt)
{
    return ds3231_rtc_get_datetime(dt);
}

esp_err_t ds3231_service_set_datetime(const ds3231_datetime_t *dt)
{
    return ds3231_rtc_set_datetime(dt);
}

esp_err_t ds3231_service_get_date(ds3231_date_t *d)
{
    return ds3231_rtc_get_date(d);
}

esp_err_t ds3231_service_set_date(const ds3231_date_t *d)
{
    return ds3231_rtc_set_date(d);
}

esp_err_t ds3231_service_set_date_only(uint8_t day, uint8_t month, uint16_t year)
{
    ds3231_date_t d = { .day = day, .month = month, .year = year };
    return ds3231_rtc_set_date(&d);
}

#endif /* CONFIG_LOCKBOX_FEATURE_DS3231 */
