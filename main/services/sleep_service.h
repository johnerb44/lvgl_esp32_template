/**
 * @file sleep_service.h
 * @brief Sleep mode service for the Secure Lockbox
 *
 * Implements a light-sleep state machine with inactivity countdown.
 * When a user signs out on the main screen, a 5-minute inactivity timeout starts.
 * During the last 60 seconds, the system shows a countdown.
 * After countdown expires:
 *   1. LCD backlight turns off
 *   2. System enters light sleep (SC16IS752 + PCA9685 stay powered)
 *   3. ESP32 RTC wakes every 20 seconds
 *   4. On each wake: polls SC16IS752 GP1 for R503 interrupt (active LOW)
 *      over 10 seconds at 1s intervals
 *   5. If R503 GP1 goes LOW at any point: fully wakes up
 *   6. If GP1 stays HIGH for 10 seconds: back to light sleep for 20s
 *
 * All sleep/timer state is cleared when the user navigates away from
 * the main screen. Sleep mode is ONLY initiated from the main screen.
 */

#ifndef SLEEP_SERVICE_H
#define SLEEP_SERVICE_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Sleep state machine states
 */
typedef enum {
    SLEEP_STATE_ACTIVE = 0,       /**< Normal operation, no sleep pending */
    SLEEP_STATE_COUNTDOWN,        /**< Inactivity timeout reached, showing countdown */
    SLEEP_STATE_LIGHT_SLEEP,      /**< Currently in light sleep (waking on RTC) */
    SLEEP_STATE_PRE_SLEEP,        /**< Countdown finished, powering down peripherals */
} sleep_state_t;

/**
 * @brief Wake-up reason after sleep exit
 */
typedef enum {
    WAKE_REASON_UNKNOWN = 0,
    WAKE_REASON_R503_INTERRUPT,          /**< R503 TOUCH/INT (GP1 went LOW) detected */
    WAKE_REASON_COUNTDOWN,               /**< Countdown reached zero (no R503 activity) */
    WAKE_REASON_LIGHT_SLEEP_ABORT,       /**< Light sleep aborted (R503 or user activity) */
} wake_reason_t;

/**
 * @brief Result of sleep entry condition check
 */
typedef struct {
    sleep_state_t state;          /**< Current sleep state */
    bool lid_closed;              /**< true if lid is closed */
    bool user_logged_out;         /**< true if no user is authenticated */
    bool face_module_stowed;      /**< true if face module is stowed */
    bool lock_engaged;            /**< true if lock is engaged */
    uint16_t countdown_seconds;   /**< Remaining countdown seconds (0 when active) */
} sleep_check_result_t;

/* ---- Public API ---- */

/**
 * @brief Initialize sleep service
 * @return ESP_OK on success
 */
esp_err_t sleep_service_init(void);

/**
 * @brief Get current sleep state
 * @return Current sleep state
 */
sleep_state_t sleep_service_get_state(void);

/**
 * @brief Get the wake reason (valid after sleep exit)
 * @return Wake reason type
 */
wake_reason_t sleep_service_get_wake_reason(void);

/**
 * @brief Tick called from UI task every ~1 second while on main screen.
 *
 * Handles:
 *   - Inactivity timer counting up after user signs out
 *   - Starting 60-second countdown after 5-minute inactivity timeout
 *   - Polling SC16IS752 GP1 during countdown for R503 interrupt (active LOW)
 *   - Triggering sleep entry when countdown reaches zero
 */
void sleep_service_tick(void);

/**
 * @brief Called on any user activity to reset inactivity timer
 */
void sleep_service_activity_detected(void);

/**
 * @brief Check if all sleep entry conditions are met.
 * @param result Output struct populated with condition values
 * @return ESP_OK if conditions can be evaluated
 */
esp_err_t sleep_service_check_entry(sleep_check_result_t *result);

/**
 * @brief Attempt to enter sleep mode.
 *
 * Powers down LCD backlight, then enters light sleep.
 * RTC wakes the ESP32 every 20 seconds to poll SC16IS752 GP1.
 * @return ESP_OK if sleep initiated, ESP_FAIL otherwise
 */
esp_err_t sleep_service_enter(void);

/**
 * @brief Wake from sleep: abort light-sleep polling cycle and restore UI.
 *
 * Also called on power-on to detect if the unit came back from a previous sleep.
 * @return ESP_OK on success
 */
esp_err_t sleep_service_wake(void);

/**
 * @brief Called when user navigates AWAY from main screen.
 *        Cancels any pending countdown/timer, resets sleep state.
 */
void sleep_service_cancel(void);

/**
 * @brief Poll SC16IS752 GP1 for R503 interrupt detection.
 *
 * Per sleep_mode.md: wake every 20s via RTC, then poll GP1 for 10s at 1s intervals.
 * GP1 is ACTIVE LOW — LOW = R503 touched (wake up), HIGH = idle (stay in sleep).
 *
 * @return true if R503 interrupt detected (GP1 went LOW)
 */
bool sleep_service_poll_r503(void);

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_SERVICE_H */
