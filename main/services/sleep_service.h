/**
 * @file sleep_service.h
 * @brief Sleep mode service for the Secure Lockbox
 *
 * Implements a sleep state machine with inactivity countdown.
 * When all entry conditions are met (lid closed, user logged out,
 * face module stowed, lock engaged) for a configurable period,
 * the system enters deep sleep to conserve battery.
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
    SLEEP_STATE_ACTIVE = 0,       /**< Normal operation */
    SLEEP_STATE_COUNTDOWN,        /**< Pre-sleep countdown active */
    SLEEP_STATE_DEEP_SLEEP,       /**< Currently deep-sleeping (never returned here) */
} sleep_state_t;

/**
 * @brief Result of sleep entry condition check
 */
typedef struct {
    sleep_state_t state;          /**< Current sleep state */
    bool lid_closed;              /**< true if lid is closed */
    bool user_logged_out;         /**< true if no user is authenticated */
    bool face_module_stowed;      /**< true if face module is stowed */
    bool lock_engaged;            /**< true if lock is engaged */
    uint16_t countdown_seconds;   /**< Remaining countdown seconds (0 during countdown) */
} sleep_check_result_t;

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
 * @brief Tick called from UI task every ~1 second
 *        Monitors inactivity and manages countdown timer.
 */
void sleep_service_tick(void);

/**
 * @brief Called on any user activity to reset inactivity timer
 */
void sleep_service_activity_detected(void);

/**
 * @brief Check if all sleep entry conditions are met
 * @param result Output struct populated with condition values
 * @return ESP_OK if conditions can be evaluated,
 *         ESP_ERR_NOT_SUPPORTED if STATUS_INPUTS feature is disabled
 */
esp_err_t sleep_service_check_entry(sleep_check_result_t *result);

/**
 * @brief Attempt to enter sleep mode.
 *        Must be called after check_entry confirms all conditions.
 *        Turns off backlight and calls esp_deep_sleep_start().
 * @return ESP_OK if sleep initiated (caller will not return),
 *         ESP_ERR_NOT_READY if conditions not met
 */
esp_err_t sleep_service_enter(void);

/**
 * @brief Wake from sleep: restore display backlight
 */
void sleep_service_wake(void);

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_SERVICE_H */
