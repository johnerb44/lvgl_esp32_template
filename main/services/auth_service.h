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

// Failed attempt policy.  Change to -1 to allow unlimited attempts (no lockout).
#define LOCKBOX_MAX_FAILED_ATTEMPTS 10
// Lockout duration in seconds after reaching max failed attempts.
#define LOCKBOX_LOCKOUT_SECONDS     60

esp_err_t auth_service_init(void);
esp_err_t auth_service_authenticate_with_fingerprint(auth_result_t *out_result);
esp_err_t auth_service_authenticate_with_face(auth_result_t *out_result);
esp_err_t auth_service_verify_pin(int userid, const char *pin, bool *out_match);

// Attempt-tracking helpers (called by UI layer)
void auth_service_record_success(void);
void auth_service_record_failure(void);
void auth_service_reset_lockout(void);
bool auth_service_is_locked_out(void);
int  auth_service_attempts_remaining(void); // -1 = unlimited, >=0 = remaining

#ifdef __cplusplus
}
#endif

#endif // AUTH_SERVICE_H
