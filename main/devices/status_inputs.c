#include "devices/status_inputs.h"
#include "comm/sc16is752_transport.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "STATUS_INPUTS";
static bool s_initialized = false;

// Assumed SC16IS752 GPIO bit mapping for lockbox interface board.
#define STATUS_GPIO_LID_BIT         (1 << 1)  // Lid switch on GP1, active low: 0 means lid open
#define STATUS_GPIO_FACE_STOW_BIT   (1 << 2)  // Face module stow on GP2, active high
#define STATUS_GPIO_TOUCH_BIT       (1 << 3)  // Fingerprint touch on GP3, active high

esp_err_t status_inputs_init(void)
{
#if !CONFIG_LOCKBOX_INTEGRATION_ENABLE || !CONFIG_LOCKBOX_FEATURE_STATUS_INPUTS
    return ESP_ERR_NOT_SUPPORTED;
#else
    esp_err_t err = sc16is752_transport_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SC16 transport init failed: %s", esp_err_to_name(err));
        return err;
    }

    err = sc16is752_transport_gpio_init(0x00, 0x00);  // all GPIO as inputs
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SC16 GPIO init failed: %s", esp_err_to_name(err));
        return err;
    }

    s_initialized = true;
    return ESP_OK;
#endif
}

esp_err_t status_inputs_read(lockbox_status_inputs_t *out_status)
{
    if (out_status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized) {
        esp_err_t err = status_inputs_init();
        if (err != ESP_OK) {
            return err;
        }
    }

#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    out_status->lid_open = false;
    out_status->face_module_stowed = true;
    out_status->fingerprint_touch_detected = false;
    return ESP_OK;
#else
    uint8_t gpio_state = 0;
    esp_err_t err = sc16is752_transport_gpio_read(&gpio_state);
    if (err != ESP_OK) {
        return err;
    }

    // Lid signal from interface board is active low.
    out_status->lid_open = ((gpio_state & STATUS_GPIO_LID_BIT) == 0);
    out_status->face_module_stowed = ((gpio_state & STATUS_GPIO_FACE_STOW_BIT) != 0);
    out_status->fingerprint_touch_detected = ((gpio_state & STATUS_GPIO_TOUCH_BIT) != 0);
    return ESP_OK;
#endif
}
