#include "services/auth_service.h"
#include "services/fingerprint_service.h"
#include "devices/hlk_tx510_device.h"
#include "esp_log.h"

static const char *TAG = "AUTH_SERVICE";

static void reset_auth_result(auth_result_t *out_result)
{
    if (out_result) {
        out_result->authenticated = false;
        out_result->userid = -1;
        out_result->factor = AUTH_FACTOR_NONE;
    }
}

esp_err_t auth_service_init(void)
{
    esp_err_t err = fingerprint_service_init();
    if (err == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(TAG, "Fingerprint service disabled by config");
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "fingerprint_service_init failed: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Fingerprint service initialized");
    return ESP_OK;
}

esp_err_t auth_service_authenticate_with_fingerprint(auth_result_t *out_result)
{
    if (out_result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    reset_auth_result(out_result);

    fingerprint_match_result_t match = {0};
    esp_err_t err = fingerprint_service_match(&match);
    if (err != ESP_OK) {
        return err;
    }

    if (match.matched) {
        out_result->authenticated = true;
        out_result->userid = match.userid;
        out_result->factor = AUTH_FACTOR_FINGERPRINT;
        return ESP_OK;
    }

    if (match.status == R503_STATUS_NO_FINGER || match.status == R503_STATUS_NO_MATCH) {
        return ESP_ERR_NOT_FOUND;
    }
    return ESP_ERR_INVALID_RESPONSE;
}

esp_err_t auth_service_authenticate_with_face(auth_result_t *out_result)
{
    (void)hlk_tx510_device_init();
    reset_auth_result(out_result);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t auth_service_verify_pin(int userid, const char *pin, bool *out_match)
{
    (void)userid;
    (void)pin;
    if (out_match == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_match = false;
    return ESP_ERR_NOT_SUPPORTED;
}
