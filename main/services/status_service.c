#include "services/status_service.h"

esp_err_t status_service_init(void)
{
    return status_inputs_init();
}

esp_err_t status_service_get_inputs(lockbox_status_inputs_t *out_status)
{
    return status_inputs_read(out_status);
}
