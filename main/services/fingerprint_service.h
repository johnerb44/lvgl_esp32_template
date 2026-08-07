#ifndef FINGERPRINT_SERVICE_H
#define FINGERPRINT_SERVICE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "devices/r503_device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    r503_status_t status;
    bool matched;
    int userid;
    int template_id;
    uint16_t confidence;
} fingerprint_match_result_t;

esp_err_t fingerprint_service_init(void);
esp_err_t fingerprint_service_match(fingerprint_match_result_t *out_result);
esp_err_t fingerprint_service_enroll_user(int userid, int *out_template_id, r503_status_t *out_status);
esp_err_t fingerprint_service_delete_template(int template_id, r503_status_t *out_status);

/**
 * @brief Get template IDs stored on sensor that are not linked to any user.
 *
 * @param ids_out    Caller-provided array to receive unlinked template IDs.
 * @param capacity   Maximum number of IDs ids_out can hold.
 * @param count_out  Set to the number of unlinked IDs found.
 */
esp_err_t fingerprint_service_get_unlinked_templates(int *ids_out, int capacity, int *count_out);

const char *fingerprint_service_status_to_string(r503_status_t status);

#ifdef __cplusplus
}
#endif

#endif // FINGERPRINT_SERVICE_H
