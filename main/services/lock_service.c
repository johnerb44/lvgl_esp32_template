#include "services/lock_service.h"
#include "devices/pca9685_device.h"

static lock_state_t s_lock_state = {
    .is_locked = true,
};

esp_err_t lock_service_init(void)
{
    return pca9685_device_init();
}

esp_err_t lock_service_lock(lock_state_t *out_state)
{
    esp_err_t err = pca9685_device_set_lock_position(LOCK_SERVO_POSITION_LOCKED);
    if (err == ESP_OK) {
        s_lock_state.is_locked = true;
    }
    if (out_state) {
        *out_state = s_lock_state;
    }
    return err == ESP_OK ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}

esp_err_t lock_service_unlock(lock_state_t *out_state)
{
    esp_err_t err = pca9685_device_set_lock_position(LOCK_SERVO_POSITION_UNLOCKED);
    if (err == ESP_OK) {
        s_lock_state.is_locked = false;
    }
    if (out_state) {
        *out_state = s_lock_state;
    }
    return err == ESP_OK ? ESP_OK : ESP_ERR_NOT_SUPPORTED;
}

esp_err_t lock_service_get_state(lock_state_t *out_state)
{
    if (out_state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_state = s_lock_state;
    return ESP_OK;
}
