#ifndef FACE_SERVICE_H
#define FACE_SERVICE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "devices/hlk_tx510_device.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    hlk_tx510_status_t status;
    bool matched;
    int userid;   // resolved from user store (-1 if not in user store)
    int face_id;  // face ID from module
} face_match_result_t;

esp_err_t face_service_init(void);
esp_err_t face_service_match(face_match_result_t *out_result);
esp_err_t face_service_enroll_user(int userid, int *out_face_id);
esp_err_t face_service_delete_face(int face_id);
const char *face_service_status_to_string(hlk_tx510_status_t status);

#ifdef __cplusplus
}
#endif

#endif // FACE_SERVICE_H
