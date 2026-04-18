#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUTH_FACTOR_NONE = 0,
    AUTH_FACTOR_FINGERPRINT = 1,
    AUTH_FACTOR_FACE = 2,
} auth_factor_t;

typedef struct {
    bool authenticated;
    int userid;
    auth_factor_t factor;
} auth_result_t;

esp_err_t auth_service_init(void);
esp_err_t auth_service_authenticate_with_fingerprint(auth_result_t *out_result);
esp_err_t auth_service_authenticate_with_face(auth_result_t *out_result);
esp_err_t auth_service_verify_pin(int userid, const char *pin, bool *out_match);

#ifdef __cplusplus
}
#endif

#endif // AUTH_SERVICE_H
