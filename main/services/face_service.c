#include "services/face_service.h"
#include "user_store.h"
#include "esp_log.h"

static const char *TAG = "FACE_SVC";

static int resolve_userid_by_faceid(int face_id)
{
    user_list_t list = {0};
    if (user_store_load(&list) != ESP_OK) {
        return -1;
    }

    int resolved = -1;
    for (size_t i = 0; i < list.count; i++) {
        if (list.items[i].faceid == face_id) {
            resolved = list.items[i].userid;
            break;
        }
    }
    user_store_free(&list);
    return resolved;
}

static esp_err_t update_user_faceid(int userid, int face_id)
{
    user_list_t list = {0};
    esp_err_t err = user_store_load(&list);
    if (err != ESP_OK) {
        return err;
    }

    int idx = -1;
    for (size_t i = 0; i < list.count; i++) {
        if (list.items[i].userid == userid) {
            idx = (int)i;
            break;
        }
    }
    if (idx < 0) {
        user_store_free(&list);
        return ESP_ERR_NOT_FOUND;
    }

    list.items[idx].faceid = face_id;
    err = user_store_save(&list);
    user_store_free(&list);
    return err;
}

static esp_err_t clear_users_for_faceid(int face_id)
{
    user_list_t list = {0};
    esp_err_t err = user_store_load(&list);
    if (err != ESP_OK) {
        return err;
    }

    bool updated = false;
    for (size_t i = 0; i < list.count; i++) {
        if (list.items[i].faceid == face_id) {
            list.items[i].faceid = -1;
            updated = true;
        }
    }
    if (updated) {
        err = user_store_save(&list);
    }
    user_store_free(&list);
    return err;
}

esp_err_t face_service_init(void)
{
    return hlk_tx510_device_init();
}

esp_err_t face_service_is_available(void)
{
    return hlk_tx510_device_ping();
}

esp_err_t face_service_match(face_match_result_t *out_result)
{
    if (out_result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    out_result->status = HLK_TX510_STATUS_COMM_ERROR;
    out_result->matched = false;
    out_result->userid = -1;
    out_result->face_id = -1;

    hlk_tx510_match_result_t raw = {0};
    esp_err_t err = hlk_tx510_device_match(&raw);
    if (err != ESP_OK) {
        out_result->status = raw.status;
        return err;
    }

    out_result->status = raw.status;
    out_result->matched = raw.matched;
    out_result->face_id = raw.matched_face_id;

    if (raw.matched && raw.matched_face_id >= 0) {
        int uid = resolve_userid_by_faceid(raw.matched_face_id);
        if (uid >= 0) {
            out_result->userid = uid;
            ESP_LOGI(TAG, "Face match: face_id=%d → userid=%d", raw.matched_face_id, uid);
        } else {
            // Face recognized by module but not linked to any user — deny access
            out_result->matched = false;
            out_result->status = HLK_TX510_STATUS_NO_MATCH;
            ESP_LOGW(TAG, "Face match: face_id=%d not linked to any user (enroll via admin)", raw.matched_face_id);
        }
    }
    return ESP_OK;
}

esp_err_t face_service_enroll_user(int userid, int *out_face_id)
{
    int face_id = -1;
    esp_err_t err = hlk_tx510_device_enroll(userid, &face_id);
    if (out_face_id) *out_face_id = face_id;
    if (err != ESP_OK) {
        return err;
    }
    if (face_id < 0) {
        return ESP_FAIL;
    }

    err = update_user_faceid(userid, face_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to persist face enrollment for user %d", userid);
        return err;
    }
    return ESP_OK;
}

esp_err_t face_service_delete_face(int face_id)
{
    esp_err_t err = hlk_tx510_device_delete_face(face_id);
    if (err != ESP_OK) {
        return err;
    }

    err = clear_users_for_faceid(face_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed clearing user mapping for face_id %d", face_id);
        return err;
    }
    return ESP_OK;
}

const char *face_service_status_to_string(hlk_tx510_status_t status)
{
    return hlk_tx510_status_to_string(status);
}
