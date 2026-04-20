#include "app/lockbox_app.h"
#include "services/auth_service.h"
#include "services/lock_service.h"
#include "services/status_service.h"
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

#if CONFIG_LOCKBOX_FEATURE_HLK_TX510
    err = face_service_init();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "face_service_init returned %s", esp_err_to_name(err));
    }
#endif

    return ESP_OK;
}
