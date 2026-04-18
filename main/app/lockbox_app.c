#include "app/lockbox_app.h"
#include "services/auth_service.h"
#include "services/lock_service.h"
#include "services/status_service.h"
#include "esp_log.h"

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

    return ESP_OK;
}
