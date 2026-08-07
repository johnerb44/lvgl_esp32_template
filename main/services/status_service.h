#ifndef STATUS_SERVICE_H
#define STATUS_SERVICE_H

#include "esp_err.h"
#include "devices/status_inputs.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t status_service_init(void);
esp_err_t status_service_get_inputs(lockbox_status_inputs_t *out_status);

#ifdef __cplusplus
}
#endif

#endif // STATUS_SERVICE_H
