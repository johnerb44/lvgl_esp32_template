#include "devices/hlk_tx510_device.h"
#include "comm/sc16is752_transport.h"

esp_err_t hlk_tx510_device_init(void)
{
    return sc16is752_transport_init();
}

esp_err_t hlk_tx510_device_match(hlk_tx510_match_result_t *result)
{
    if (result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    result->matched = false;
    result->matched_userid = -1;
    result->matched_face_id = -1;
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t hlk_tx510_device_enroll(int userid, int *out_face_id)
{
    (void)userid;
    if (out_face_id) {
        *out_face_id = -1;
    }
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t hlk_tx510_device_delete_face(int face_id)
{
    (void)face_id;
    return ESP_ERR_NOT_SUPPORTED;
}
