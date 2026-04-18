#ifndef STATUS_INPUTS_H
#define STATUS_INPUTS_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool lid_open;
    bool face_module_stowed;
    bool fingerprint_touch_detected;
} lockbox_status_inputs_t;

esp_err_t status_inputs_init(void);
esp_err_t status_inputs_read(lockbox_status_inputs_t *out_status);

#ifdef __cplusplus
}
#endif

#endif // STATUS_INPUTS_H
