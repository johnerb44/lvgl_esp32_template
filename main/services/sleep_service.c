/**
 * @file sleep_service.c
 * @brief Sleep mode service implementation
 *
 * Implements a sleep state machine with inactivity countdown.
 * Entry conditions (all must be true):
 *   1. Lid closed
 *   2. User logged out
 *   3. Face module stowed
 *   4. Lock engaged
 * After a configurable inactivity timeout, a 60-second countdown begins.
 * If all conditions remain true at countdown end, the system enters deep sleep.
 */

#include "sleep_service.h"
#include "devices/status_inputs.h"
#include "services/lock_service.h"
#include "services/session_service.h"
#include "ch422g_driver.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "SLEEP_SERVICE";

/* ---- Internal state ---- */
static sleep_state_t s_state = SLEEP_STATE_ACTIVE;
static int64_t s_last_activity_us = 0;
static uint16_t s_countdown_remaining = 0;

#if CONFIG_LOCKBOX_FEATURE_SLEEP_MODE
static const uint16_t s_timeout_s = CONFIG_LOCKBOX_SLEEP_INACTIVITY_TIMEOUT_S;
#else
static const uint16_t s_timeout_s = 300; /* default 5 min */
#endif

#define SLEEP_COUNTDOWN_SECONDS 60

/* ---- Forward declarations ---- */
static esp_err_t sleep_service_check_conditions(bool *p_lid_closed, bool *p_face_stowed);
static void set_state(sleep_state_t new_state);

/* ---- Public API ---- */

esp_err_t sleep_service_init(void)
{
    s_last_activity_us = esp_timer_get_time();
    s_state = SLEEP_STATE_ACTIVE;
    s_countdown_remaining = 0;
    ESP_LOGI(TAG, "Sleep service initialized (timeout=%ds)", s_timeout_s);
    return ESP_OK;
}

sleep_state_t sleep_service_get_state(void)
{
    return s_state;
}

void sleep_service_activity_detected(void)
{
    if (s_state != SLEEP_STATE_ACTIVE) {
        return;
    }
    s_last_activity_us = esp_timer_get_time();
    s_countdown_remaining = 0;
    ESP_LOGD(TAG, "Activity detected, inactivity timer reset");
}

void sleep_service_tick(void)
{
    if (s_state == SLEEP_STATE_ACTIVE) {
        int64_t elapsed_us = esp_timer_get_time() - s_last_activity_us;
        int64_t elapsed_s = elapsed_us / 1000000;

        if (elapsed_s >= s_timeout_s) {
            ESP_LOGI(TAG, "Inactivity timeout reached, starting %ds countdown",
                     SLEEP_COUNTDOWN_SECONDS);
            set_state(SLEEP_STATE_COUNTDOWN);
            s_countdown_remaining = SLEEP_COUNTDOWN_SECONDS;
        }
    } else if (s_state == SLEEP_STATE_COUNTDOWN) {
        s_countdown_remaining--;

        if (s_countdown_remaining == 0) {
            /* Check entry conditions before entering sleep */
            sleep_check_result_t result = {0};
            if (sleep_service_check_entry(&result) == ESP_OK) {
                ESP_LOGI(TAG, "All sleep entry conditions met, entering sleep");
                set_state(SLEEP_STATE_DEEP_SLEEP);
            } else {
                /* Conditions not met, return to active */
                ESP_LOGD(TAG, "Sleep entry conditions not met, returning to active");
                set_state(SLEEP_STATE_ACTIVE);
                s_last_activity_us = esp_timer_get_time();
                s_countdown_remaining = 0;
            }
        }
    }
}

esp_err_t sleep_service_check_entry(sleep_check_result_t *result)
{
    if (!result) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(result, 0, sizeof(sleep_check_result_t));
    result->state = s_state;

    /* User logged out is always true during countdown (user has signed out) */
    result->user_logged_out = !session_service_is_authenticated();

    /* Lock engaged check */
    lock_state_t lock_state = {0};
    esp_err_t err = lock_service_get_state(&lock_state);
    result->lock_engaged = (err == ESP_OK) && lock_state.is_locked;

#if CONFIG_LOCKBOX_FEATURE_STATUS_INPUTS
    lockbox_status_inputs_t inputs = {0};
    err = status_inputs_read(&inputs);
    result->lid_closed = (err == ESP_OK) && !inputs.lid_open;
    result->face_module_stowed = (err == ESP_OK) && inputs.face_module_stowed;
#else
    /* Mock: assume conditions met when STATUS_INPUTS is disabled */
    result->lid_closed = true;
    result->face_module_stowed = true;
#endif

    return ESP_OK;
}

esp_err_t sleep_service_enter(void)
{
    if (s_state != SLEEP_STATE_COUNTDOWN) {
        return ESP_FAIL;
    }

    /* Turn off LCD backlight */
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE
    /* I2C port 0 is used for the CH422G on the Waveshare board */
    ch422g_backlight_control(0, false);
#endif

    ESP_LOGI(TAG, "Entering deep sleep");
    esp_deep_sleep_start();

    /* Should never reach here */
    return ESP_FAIL;
}

void sleep_service_wake(void)
{
    ESP_LOGI(TAG, "Waking from deep sleep");
    s_state = SLEEP_STATE_ACTIVE;
    s_countdown_remaining = 0;
    s_last_activity_us = esp_timer_get_time();

    /* Restore backlight */
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE
    ch422g_backlight_control(0, true);
#endif

    ESP_LOGI(TAG, "Backlight restored, system active");
}

/* ---- Internal helpers ---- */

static void set_state(sleep_state_t new_state)
{
    sleep_state_t old = s_state;
    s_state = new_state;
    if (old != new_state) {
        ESP_LOGI(TAG, "State: %s -> %s",
                 old == SLEEP_STATE_ACTIVE ? "ACTIVE" :
                 old == SLEEP_STATE_COUNTDOWN ? "COUNTDOWN" : "DEEP_SLEEP",
                 new_state == SLEEP_STATE_ACTIVE ? "ACTIVE" :
                 new_state == SLEEP_STATE_COUNTDOWN ? "COUNTDOWN" : "DEEP_SLEEP");
    }
}
