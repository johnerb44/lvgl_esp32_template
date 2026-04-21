#include "services/session_service.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "SESSION_SERVICE";

static user_t s_current_user  = {0};
static bool   s_authenticated  = false;

esp_err_t session_service_set_user(const user_t *user)
{
    if (user == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    s_current_user  = *user;
    s_authenticated = true;
    ESP_LOGI(TAG, "User logged in: %s (userid=%d, admin=%s)",
             s_current_user.username,
             s_current_user.userid,
             s_current_user.admin ? "true" : "false");
    return ESP_OK;
}

esp_err_t session_service_get_user(user_t *out_user)
{
    if (out_user == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_authenticated) {
        return ESP_ERR_NOT_FOUND;
    }
    *out_user = s_current_user;
    return ESP_OK;
}

bool session_service_is_authenticated(void)
{
    return s_authenticated;
}

int session_service_get_userid(void)
{
    return s_authenticated ? s_current_user.userid : -1;
}

void session_service_clear(void)
{
    memset(&s_current_user, 0, sizeof(s_current_user));
    s_authenticated = false;
    ESP_LOGI(TAG, "Session cleared");
}
