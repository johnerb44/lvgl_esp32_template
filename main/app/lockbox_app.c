#include "app/lockbox_app.h"
#include "services/auth_service.h"
#include "services/lock_service.h"
#include "services/status_service.h"
#include "services/battery_service.h"
#include "services/ina219_service.h"
#include "services/sleep_service.h"
#if CONFIG_LOCKBOX_FEATURE_DS3231
#include "services/ds3231_service.h"
#endif
#include "esp_log.h"
#include "sdkconfig.h"

#if CONFIG_LOCKBOX_FEATURE_HLK_TX510
#include "services/face_service.h"
#endif

static const char *TAG = "LOCKBOX_APP";

esp_err_t lockbox_app_init(void)
{
    esp_err_t err = auth_service_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "auth_service_init returned %s", esp_err_to_name(err));
    }

    err = lock_service_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "lock_service_init returned %s", esp_err_to_name(err));
    }

    err = status_service_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "status_service_init returned %s", esp_err_to_name(err));
    }

#if CONFIG_LOCKBOX_FEATURE_DS3231
    ESP_LOGI(TAG, "Initializing DS3231 RTC service");
    err = ds3231_service_init(0, 0x68);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ds3231_service_init returned %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "DS3231 RTC service initialized");
    }
#endif

#if CONFIG_LOCKBOX_FEATURE_SLEEP_MODE
    ESP_LOGI(TAG, "Initializing sleep service");
    err = sleep_service_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "sleep_service_init returned %s", esp_err_to_name(err));
    }
#endif

#if CONFIG_LOCKBOX_FEATURE_INA219
    ESP_LOGI(TAG, "Initializing INA219 service for battery monitoring");
    // Use the default I2C port and address for INA219
    err = ina219_service_init(0, 0x41);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "ina219_service_init returned %s", esp_err_to_name(err));
        // Continue initialization as battery monitoring is not critical for core functionality
    } else {
        ESP_LOGI(TAG, "INA219 service initialized successfully");
    }
#endif


#if CONFIG_LOCKBOX_FEATURE_HLK_TX510
    err = face_service_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "face_service_init returned %s", esp_err_to_name(err));
    }
#endif

    return ESP_OK;
}
