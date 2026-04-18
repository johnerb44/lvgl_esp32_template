#ifndef R503_DEVICE_H
#define R503_DEVICE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    R503_STATUS_OK = 0,
    R503_STATUS_NO_FINGER,
    R503_STATUS_NO_MATCH,
    R503_STATUS_BAD_LOCATION,
    R503_STATUS_ENROLL_MISMATCH,
    R503_STATUS_DELETE_FAILED,
    R503_STATUS_SENSOR_ERROR,
    R503_STATUS_TIMEOUT,
    R503_STATUS_COMM_ERROR,
} r503_status_t;

typedef struct {
    r503_status_t status;
    bool matched;
    int matched_userid;
    int matched_template_id;
    uint16_t confidence;
} r503_match_result_t;

esp_err_t r503_device_init(void);
esp_err_t r503_device_match(r503_match_result_t *result);
esp_err_t r503_device_enroll(int userid, int *out_template_id, r503_status_t *out_status);
esp_err_t r503_device_delete_template(int template_id, r503_status_t *out_status);
const char *r503_status_to_string(r503_status_t status);

#ifdef __cplusplus
}
#endif

#endif // R503_DEVICE_H
