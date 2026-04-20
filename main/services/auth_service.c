#include "services/auth_service.h"
#include "services/fingerprint_service.h"
#include "devices/hlk_tx510_device.h"
#include "user_store.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "AUTH_SERVICE";

static int  s_failed_attempts = 0;
static bool s_locked_out = false;

static void reset_auth_result(auth_result_t *out_result)
{
    if (out_result) {
        out_result->authenticated = false;
        out_result->userid = -1;
        out_result->factor = AUTH_FACTOR_NONE;
    }
}

// ── Attempt tracking ─────────────────────────────────────────────────────────

void auth_service_record_success(void)
{
    s_failed_attempts = 0;
    s_locked_out = false;
}

void auth_service_record_failure(void)
{
    if (LOCKBOX_MAX_FAILED_ATTEMPTS < 0) {
        return; // unlimited — never lock out
    }
    s_failed_attempts++;
    if (s_failed_attempts >= LOCKBOX_MAX_FAILED_ATTEMPTS) {
        s_locked_out = true;
        ESP_LOGW(TAG, "Lockout triggered after %d failed attempts", s_failed_attempts);
    }
}

void auth_service_reset_lockout(void)
{
    s_failed_attempts = 0;
    s_locked_out = false;
    ESP_LOGI(TAG, "Lockout reset");
}

bool auth_service_is_locked_out(void)
{
    return s_locked_out;
}

int auth_service_attempts_remaining(void)
{
    if (LOCKBOX_MAX_FAILED_ATTEMPTS < 0) {
        return -1; // unlimited
    }
    int remaining = LOCKBOX_MAX_FAILED_ATTEMPTS - s_failed_attempts;
    return remaining < 0 ? 0 : remaining;
}

// ── Service init ─────────────────────────────────────────────────────────────

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
    if (out_match == NULL || pin == NULL || userid < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_match = false;

    user_list_t list = {0};
    esp_err_t err = user_store_load(&list);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "user_store_load failed: %s", esp_err_to_name(err));
        return err;
    }

    bool found = false;
    for (size_t i = 0; i < list.count; i++) {
        if (list.items[i].userid == userid) {
            *out_match = (strcmp(list.items[i].pin, pin) == 0);
            found = true;
            break;
        }
    }
    user_store_free(&list);

    if (!found) {
        ESP_LOGW(TAG, "verify_pin: userid %d not found", userid);
        return ESP_ERR_NOT_FOUND;
    }
    return ESP_OK;
}

