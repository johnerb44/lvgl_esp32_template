#ifndef HLK_TX510_DEVICE_H
#define HLK_TX510_DEVICE_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool matched;
    int matched_userid;
    int matched_face_id;
} hlk_tx510_match_result_t;

esp_err_t hlk_tx510_device_init(void);
esp_err_t hlk_tx510_device_match(hlk_tx510_match_result_t *result);
esp_err_t hlk_tx510_device_enroll(int userid, int *out_face_id);
esp_err_t hlk_tx510_device_delete_face(int face_id);

#ifdef __cplusplus
}
#endif

#endif // HLK_TX510_DEVICE_H
