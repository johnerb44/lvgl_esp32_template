#ifndef HLK_TX510_DEVICE_H
#define HLK_TX510_DEVICE_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HLK_TX510_STATUS_OK          = 0x00,  // Recognized successfully
    HLK_TX510_STATUS_NO_FACE     = 0x01,  // No face detected
    HLK_TX510_STATUS_POSE_ERROR  = 0x03,  // Face pose angle too large
    HLK_TX510_STATUS_2D_LIVENESS = 0x06,  // 2D liveness check failed
    HLK_TX510_STATUS_3D_LIVENESS = 0x07,  // 3D liveness check failed
    HLK_TX510_STATUS_NO_MATCH    = 0x08,  // Face not in database
    HLK_TX510_STATUS_DUPLICATE   = 0x09,  // Face already enrolled
    HLK_TX510_STATUS_SAVE_ERROR  = 0x0A,  // Storage error during enroll
    HLK_TX510_STATUS_COMM_ERROR  = 0xFE,  // Transport/framing error
    HLK_TX510_STATUS_TIMEOUT     = 0xFF,  // No response within timeout
} hlk_tx510_status_t;

typedef struct {
    hlk_tx510_status_t status;
    bool matched;
    int matched_userid;   // resolved from user store (-1 if not found)
    int matched_face_id;  // face ID returned by the module
} hlk_tx510_match_result_t;

const char *hlk_tx510_status_to_string(hlk_tx510_status_t status);

esp_err_t hlk_tx510_device_init(void);
esp_err_t hlk_tx510_device_match(hlk_tx510_match_result_t *result);
esp_err_t hlk_tx510_device_enroll(int userid, int *out_face_id);
esp_err_t hlk_tx510_device_delete_face(int face_id);

#ifdef __cplusplus
}
#endif

#endif // HLK_TX510_DEVICE_H
