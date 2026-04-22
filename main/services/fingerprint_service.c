#include "services/fingerprint_service.h"
#include "user_store.h"
#include "esp_log.h"

#define FINGERPRINT_SERVICE_MATCH_RETRIES 3

static const char *TAG = "FINGERPRINT_SVC";

static int resolve_userid_by_template(int template_id)
{
    user_list_t list = {0};
    if (user_store_load(&list) != ESP_OK) {
        return -1;
    }

    int resolved_userid = -1;
    for (size_t i = 0; i < list.count; i++) {
        if (list.items[i].fingerid == template_id) {
            resolved_userid = list.items[i].userid;
            break;
        }
    }
    user_store_free(&list);
    return resolved_userid;
}

static esp_err_t update_user_fingerprint_id(int userid, int fingerid)
{
    user_list_t list = {0};
    esp_err_t err = user_store_load(&list);
    if (err != ESP_OK) {
        return err;
    }

    int matched_index = -1;
    for (size_t i = 0; i < list.count; i++) {
        if (list.items[i].userid == userid) {
            matched_index = (int)i;
            break;
        }
    }

    if (matched_index < 0) {
        user_store_free(&list);
        return ESP_ERR_NOT_FOUND;
    }

    list.items[matched_index].fingerid = fingerid;
    err = user_store_save(&list);
    user_store_free(&list);
    return err;
}

static esp_err_t clear_users_for_template_id(int template_id)
{
    user_list_t list = {0};
    esp_err_t err = user_store_load(&list);
    if (err != ESP_OK) {
        return err;
    }

    bool updated = false;
    for (size_t i = 0; i < list.count; i++) {
        if (list.items[i].fingerid == template_id) {
            list.items[i].fingerid = -1;
            updated = true;
        }
    }

    if (updated) {
        err = user_store_save(&list);
    }

    user_store_free(&list);
    return err;
}

esp_err_t fingerprint_service_init(void)
{
    return r503_device_init();
}

esp_err_t fingerprint_service_match(fingerprint_match_result_t *out_result)
{
    if (out_result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    out_result->status = R503_STATUS_NO_MATCH;
    out_result->matched = false;
    out_result->userid = -1;
    out_result->template_id = -1;
    out_result->confidence = 0;

    r503_match_result_t raw = {0};
    esp_err_t err = ESP_FAIL;
    for (int attempt = 0; attempt < FINGERPRINT_SERVICE_MATCH_RETRIES; attempt++) {
        err = r503_device_match(&raw);
        if (err == ESP_OK) {
            if (raw.status == R503_STATUS_TIMEOUT ||
                raw.status == R503_STATUS_COMM_ERROR ||
                raw.status == R503_STATUS_SENSOR_ERROR) {
                continue;
            }
            break;
        }
    }
    if (err != ESP_OK) {
        out_result->status = R503_STATUS_COMM_ERROR;
        return err;
    }

    out_result->status = raw.status;
    out_result->matched = raw.matched;
    out_result->template_id = raw.matched_template_id;
    out_result->confidence = raw.confidence;

    if (raw.matched && raw.matched_template_id >= 0) {
        int resolved_userid = resolve_userid_by_template(raw.matched_template_id);
        if (resolved_userid >= 0) {
            out_result->userid = resolved_userid;
        } else {
            ESP_LOGW(TAG, "FP match: template_id=%d not linked to any user — treating as no match",
                     raw.matched_template_id);
            out_result->matched = false;
        }
    }
    return ESP_OK;
}

esp_err_t fingerprint_service_enroll_user(int userid, int *out_template_id, r503_status_t *out_status)
{
    int template_id = -1;
    r503_status_t status = R503_STATUS_SENSOR_ERROR;

    esp_err_t err = r503_device_enroll(userid, &template_id, &status);
    if (out_status != NULL) {
        *out_status = status;
    }
    if (out_template_id != NULL) {
        *out_template_id = template_id;
    }
    if (err != ESP_OK) {
        return err;
    }
    if (status != R503_STATUS_OK || template_id < 0) {
        return ESP_OK;
    }

    err = update_user_fingerprint_id(userid, template_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to persist fingerprint enrollment for user %d", userid);
        return err;
    }
    return ESP_OK;
}

esp_err_t fingerprint_service_delete_template(int template_id, r503_status_t *out_status)
{
    r503_status_t status = R503_STATUS_SENSOR_ERROR;
    esp_err_t err = r503_device_delete_template(template_id, &status);
    if (out_status != NULL) {
        *out_status = status;
    }
    if (err != ESP_OK) {
        return err;
    }
    if (status != R503_STATUS_OK) {
        return ESP_OK;
    }

    err = clear_users_for_template_id(template_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed clearing user mapping for template %d", template_id);
        return err;
    }
    return ESP_OK;
}

const char *fingerprint_service_status_to_string(r503_status_t status)
{
    return r503_status_to_string(status);
}
