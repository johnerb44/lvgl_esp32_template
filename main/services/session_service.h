#ifndef SESSION_SERVICE_H
#define SESSION_SERVICE_H

#include <stdbool.h>
#include "esp_err.h"
#include "user_store.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t session_service_set_user(const user_t *user);
esp_err_t session_service_get_user(user_t *out_user);
bool      session_service_is_authenticated(void);
int       session_service_get_userid(void);
void      session_service_clear(void);

#ifdef __cplusplus
}
#endif

#endif // SESSION_SERVICE_H
