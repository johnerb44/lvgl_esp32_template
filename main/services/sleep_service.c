/**
 * @file sleep_service.c
 * @brief Sleep mode service implementation
 *
 * Implements blocking sleep cycle with inactivity countdown:
 * - 5-minute inactivity timeout (SLEEP_INACTIVITY_TIMEOUT_SEC)
 * - 60-second countdown display phase (SLEEP_COUNTDOWN_SEC)
 * - Deep sleep for 20 seconds (SLEEP_RTC_INTERVAL_SEC) with RTC timer wake
 * - R503 polling phase: 10 seconds of light sleep polls at 500ms intervals
 * - Cycle repeats (deep sleep → poll → deep sleep) until R503 touch detected
 */

#include "services/sleep_service.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "comm/sc16is752_transport.h"
#include "ch422g_driver.h"
#include "waveshare_rgb_lcd_port.h"
#include "devices/r503_device.h"
#include <inttypes.h>

#if CONFIG_LOCKBOX_FEATURE_SLEEP_MODE

static const char *TAG = "SLEEP_SERVICE";

/* ---- Sleep timing constants ---- */
#define SLEEP_INACTIVITY_TIMEOUT_SEC   300    /* 5 minutes before countdown starts */
#define SLEEP_COUNTDOWN_SEC            60     /* Last 60 seconds show countdown */
#define SLEEP_RTC_INTERVAL_SEC         20     /* Deep sleep RTC wake interval */
#define SLEEP_POLL_DURATION_SEC        10     /* Polling window duration (R503 touch detection) */
#define SLEEP_POLL_INTERVAL_MS         500    /* RTC wake interval during polling */
#define SLEEP_POLL_ITERATIONS          ((SLEEP_POLL_DURATION_SEC * 1000) / SLEEP_POLL_INTERVAL_MS)

/* ---- Sleep service state ---- */
static struct {
    sleep_state_t state;              /* Current sleep state */
    wake_reason_t wake_reason;        /* Reason for last wake */
    uint32_t inactivity_timer_sec;    /* Elapsed inactivity seconds */
    uint32_t countdown_timer_sec;     /* Remaining countdown seconds */
} s_sleep_state = {
    .state = SLEEP_STATE_ACTIVE,
    .wake_reason = WAKE_REASON_UNKNOWN,
    .inactivity_timer_sec = 0,
    .countdown_timer_sec = 0,
};

/* Forward declarations */
static bool read_r503_interrupt_pin(void);

/**
 * @brief Initialize sleep service
 */
esp_err_t sleep_service_init(void)
{
    ESP_LOGI(TAG, "Sleep service initialized");
    
    /* Initialize SC16IS752 GPIO for reading R503 interrupt (GP1) */
    esp_err_t ret = sc16is752_transport_gpio_init(0x00, 0xFF);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to init SC16IS752 GPIO: %s", esp_err_to_name(ret));
        /* Not fatal — continue without R503 interrupt detection */
    }
    
    s_sleep_state.state = SLEEP_STATE_ACTIVE;
    s_sleep_state.wake_reason = WAKE_REASON_UNKNOWN;
    s_sleep_state.inactivity_timer_sec = 0;
    
    return ESP_OK;
}

/**
 * @brief Get current sleep state
 */
sleep_state_t sleep_service_get_state(void)
{
    return s_sleep_state.state;
}

/**
 * @brief Get wake reason
 */
wake_reason_t sleep_service_get_wake_reason(void)
{
    return s_sleep_state.wake_reason;
}

/**
 * @brief Called every ~1 second from UI main screen task
 *
 * Handles inactivity timeout counting and countdown.
 */
void sleep_service_tick(void)
{
    if (s_sleep_state.state == SLEEP_STATE_ACTIVE) {
        /* Count up inactivity */
        s_sleep_state.inactivity_timer_sec++;
        
        if (s_sleep_state.inactivity_timer_sec >= SLEEP_INACTIVITY_TIMEOUT_SEC) {
            /* Inactivity timeout reached: start 60-second countdown */
            ESP_LOGI(TAG, "Inactivity timeout reached, starting countdown");
            s_sleep_state.state = SLEEP_STATE_COUNTDOWN;
            s_sleep_state.countdown_timer_sec = SLEEP_COUNTDOWN_SEC;
        }
    } 
    else if (s_sleep_state.state == SLEEP_STATE_COUNTDOWN) {
        /* Count down */
        if (s_sleep_state.countdown_timer_sec > 0) {
            s_sleep_state.countdown_timer_sec--;
            ESP_LOGI(TAG, "Countdown: %" PRIu32 "s remaining", s_sleep_state.countdown_timer_sec);
            
            /* Check R503 interrupt during countdown for user activity */
            if (read_r503_interrupt_pin()) {
                ESP_LOGI(TAG, "R503 interrupt detected during countdown, resetting timers");
                sleep_service_activity_detected();
                return;
            }
        } else {
            /* Countdown finished: enter sleep cycle */
            ESP_LOGI(TAG, "All sleep entry conditions met — entering sleep");
            s_sleep_state.state = SLEEP_STATE_PRE_SLEEP;
            
            esp_err_t ret = sleep_service_enter();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "sleep_service_enter() failed: %s", esp_err_to_name(ret));
                /* Reset state on error */
                s_sleep_state.state = SLEEP_STATE_ACTIVE;
                s_sleep_state.inactivity_timer_sec = 0;
            }
            /* On success, sleep_service_enter() handles the entire sleep cycle
             * internally and only returns when R503 is detected (state set to ACTIVE). */
        }
    }
    /* DEEP_SLEEP and POLLING states are handled entirely within sleep_service_enter()
     * — the blocking sleep cycle does not return to tick() until R503 is detected. */
}

/**
 * @brief Called on any user activity to reset inactivity timer
 */
void sleep_service_activity_detected(void)
{
    ESP_LOGI(TAG, "Activity detected, resetting timers");
    s_sleep_state.state = SLEEP_STATE_ACTIVE;
    s_sleep_state.inactivity_timer_sec = 0;
    s_sleep_state.countdown_timer_sec = 0;
}

/**
 * @brief Check sleep entry conditions
 */
esp_err_t sleep_service_check_entry(sleep_check_result_t *result)
{
    if (!result) {
        return ESP_ERR_INVALID_ARG;
    }
    
    /* TODO: Check actual system conditions from services */
    /* For now, just report current state */
    result->state = s_sleep_state.state;
    result->lid_closed = true;              /* Assume lid closed */
    result->user_logged_out = true;         /* Assume logged out on main screen */
    result->face_module_stowed = true;      /* Assume stowed */
    result->lock_engaged = true;            /* Assume locked */
    result->countdown_seconds = s_sleep_state.countdown_timer_sec;
    
    return ESP_OK;
}

/**
 * @brief Attempt to enter sleep mode
 *
 * Powers down LCD backlight, waits, then enters light sleep.
 */
esp_err_t sleep_service_enter(void)
{
    if (s_sleep_state.state != SLEEP_STATE_PRE_SLEEP) {
        ESP_LOGE(TAG, "Cannot enter sleep — state is %d (expected PRE_SLEEP)",
                 s_sleep_state.state);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Shutting down LCD backlight");
    ch422g_backlight_control(I2C_MASTER_NUM, false);
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Main sleep cycle: deep sleep → R503 polling → deep sleep (repeat until R503) */
    while (true) {
        s_sleep_state.state = SLEEP_STATE_DEEP_SLEEP;

        /* ===== Phase 1: Deep sleep (20s RTC wake) ===== */
        ESP_LOGI(TAG, "Entering deep sleep with RTC wake every 20s");
        esp_err_t ret = esp_sleep_enable_timer_wakeup(
            (uint64_t)SLEEP_RTC_INTERVAL_SEC * 1000000ULL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable RTC timer: %s", esp_err_to_name(ret));
            ch422g_backlight_control(I2C_MASTER_NUM, true);
            s_sleep_state.state = SLEEP_STATE_ACTIVE;
            s_sleep_state.inactivity_timer_sec = 0;
            return ret;
        }

        ESP_LOGI(TAG, "RTC timer configured for 20s intervals");
        esp_light_sleep_start();
        ESP_LOGI(TAG, "Woke from deep sleep, checking R503 interrupt");

        /* Check R503 immediately after wake — if touched, fully wake */
        if (read_r503_interrupt_pin()) {
            ESP_LOGI(TAG, "R503 interrupt detected during deep sleep wake, waking system");
            sleep_service_wake();
            return ESP_OK;
        }

        /* ===== Phase 2: Light sleep polling (10s at 500ms intervals) ===== */
        s_sleep_state.state = SLEEP_STATE_POLLING;
        ESP_LOGI(TAG, "Starting %ds R503 polling phase (%d polls of 500ms each)",
                 SLEEP_POLL_DURATION_SEC, SLEEP_POLL_ITERATIONS);

        uint32_t poll_start_us = (uint32_t)(esp_timer_get_time() / 1000000);
        bool r503_detected = false;

        for (int i = 0; i < SLEEP_POLL_ITERATIONS; i++) {
            /* Light sleep for polling interval */
            ret = esp_sleep_enable_timer_wakeup(
                (uint64_t)SLEEP_POLL_INTERVAL_MS * 1000ULL);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to enable RTC timer for polling: %s",
                         esp_err_to_name(ret));
                break;
            }
            esp_light_sleep_start();

            /* Check R503 after each wake */
            uint32_t elapsed = (uint32_t)(esp_timer_get_time() / 1000000) - poll_start_us;
            if (read_r503_interrupt_pin()) {
                ESP_LOGI(TAG, "R503 detected after %" PRIu32 "s polling, waking system", elapsed);
                s_sleep_state.wake_reason = WAKE_REASON_R503_INTERRUPT;
                r503_detected = true;
                break;
            }

            /* Log polling progress */
            ESP_LOGI(TAG, "Poll iter #%d: elapsed %" PRIu32 "s (of %" PRIu32 "s) — no R503",
                     i + 1, elapsed, (uint32_t)SLEEP_POLL_DURATION_SEC);
        }

        if (r503_detected) {
            sleep_service_wake();
            return ESP_OK;
        }

        /* Polling complete without R503 — go back to deep sleep */
        uint32_t poll_elapsed = (uint32_t)(esp_timer_get_time() / 1000000) - poll_start_us;
        ESP_LOGI(TAG, "Polling phase complete (%" PRIu32 "s), no R503 activity, return"
                 "ing to sleep", poll_elapsed);

        /* Loop continues → re-enter deep sleep */
    }
}

/**
 * @brief Wake from sleep: restore LCD and UI
 */
esp_err_t sleep_service_wake(void)
{
    ESP_LOGI(TAG, "Waking from sleep, restoring LCD");
    
    /* Restore LCD backlight */
    ch422g_backlight_control(I2C_MASTER_NUM, true);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* Return to active state */
    s_sleep_state.state = SLEEP_STATE_ACTIVE;
    s_sleep_state.inactivity_timer_sec = 0;
    s_sleep_state.countdown_timer_sec = 0;
    
    return ESP_OK;
}

/**
 * @brief Cancel sleep mode: called when leaving main screen
 */
void sleep_service_cancel(void)
{
    ESP_LOGI(TAG, "Canceling sleep mode (leaving main screen)");
    s_sleep_state.state = SLEEP_STATE_ACTIVE;
    s_sleep_state.inactivity_timer_sec = 0;
    s_sleep_state.countdown_timer_sec = 0;
}

/* ---- Internal helper functions ---- */

/**
 * @brief Read R503 interrupt signal from SC16IS752 GP1
 *
 * Per sleep_mode.md:
 *   - GP1 is HIGH when R503 is NOT touched (idle)
 *   - GP1 is LOW when R503 IS touched (interrupt)
 *
 * @return true if interrupt detected (GP1 is LOW), false if idle (GP1 is HIGH)
 */
static bool read_r503_interrupt_pin(void)
{
    /* Check mock R503 touch state first (for testing) */
    if (r503_device_get_mock_touch()) {
        ESP_LOGI(TAG, "R503 interrupt detected via mock (R503 touch simulated)");
        return true;
    }

    /* Read real GP1 state via SC16IS752 */
    uint8_t gpio_state = 0;
    esp_err_t ret = sc16is752_transport_gpio_read(&gpio_state);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read SC16IS752 GPIO: %s", esp_err_to_name(ret));
        return false;
    }
    
    /* GP1 is bit 1 of gpio_state: LOW (bit 1 == 0) when R503 touched */
    bool gp1_state = (gpio_state & 0x02) == 0;
    
    if (gp1_state) {
        ESP_LOGI(TAG, "R503 interrupt detected on GP1");
    }
    
    return gp1_state;
}

#else  /* !CONFIG_LOCKBOX_FEATURE_SLEEP_MODE */

/* Stub implementations when feature is disabled */

esp_err_t sleep_service_init(void)
{
    return ESP_OK;
}

sleep_state_t sleep_service_get_state(void)
{
    return SLEEP_STATE_ACTIVE;
}

wake_reason_t sleep_service_get_wake_reason(void)
{
    return WAKE_REASON_UNKNOWN;
}

void sleep_service_tick(void)
{
}

void sleep_service_activity_detected(void)
{
}

esp_err_t sleep_service_check_entry(sleep_check_result_t *result)
{
    if (result) {
        result->state = SLEEP_STATE_ACTIVE;
        result->countdown_seconds = 0;
        result->lid_closed = true;
        result->user_logged_out = false;
        result->face_module_stowed = true;
        result->lock_engaged = true;
    }
    return ESP_OK;
}

esp_err_t sleep_service_enter(void)
{
    return ESP_FAIL;
}

esp_err_t sleep_service_wake(void)
{
    return ESP_OK;
}

void sleep_service_cancel(void)
{
}

#endif /* CONFIG_LOCKBOX_FEATURE_SLEEP_MODE */
