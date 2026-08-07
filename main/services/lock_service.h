#ifndef LOCK_SERVICE_H
#define LOCK_SERVICE_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool is_locked;
} lock_state_t;

esp_err_t lock_service_init(void);
esp_err_t lock_service_lock(lock_state_t *out_state);
esp_err_t lock_service_unlock(lock_state_t *out_state);
esp_err_t lock_service_get_state(lock_state_t *out_state);

#ifdef __cplusplus
}
#endif

#endif // LOCK_SERVICE_H
