/**
 * @file sleep_service.c
 * @brief Sleep mode service implementation
 *
 * Implements light-sleep state machine with inactivity countdown and RTC polling.
 * - 5-minute inactivity timeout (SLEEP_INACTIVITY_TIMEOUT_SEC)
 * - 60-second countdown display phase (SLEEP_COUNTDOWN_SEC)
 * - Light sleep entry with LCD backlight off
 * - RTC wake every 20 seconds (SLEEP_RTC_INTERVAL_SEC)
 * - SC16IS752 GP1 polling for 10 seconds (SLEEP_POLL_DURATION_SEC)
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
#include <inttypes.h>

#if CONFIG_LOCKBOX_FEATURE_SLEEP_MODE

static const char *TAG = "SLEEP_SERVICE";

/* ---- Sleep timing constants ---- */
#define SLEEP_INACTIVITY_TIMEOUT_SEC   300    /* 5 minutes before countdown starts */
#define SLEEP_COUNTDOWN_SEC            60     /* Last 60 seconds show countdown */
#define SLEEP_RTC_INTERVAL_SEC         20     /* RTC wakes every 20 seconds */
#define SLEEP_POLL_DURATION_SEC        10     /* Poll GP1 for 10 seconds on each wake */
#define SLEEP_POLL_INTERVAL_MS         500    /* Poll interval: 500ms (20 times in 10s) */

/* ---- Sleep service state ---- */
static struct {
    sleep_state_t state;              /* Current sleep state */
    wake_reason_t wake_reason;        /* Reason for last wake */
    uint32_t inactivity_timer_sec;    /* Elapsed inactivity seconds */
    uint32_t countdown_timer_sec;     /* Remaining countdown seconds */
    uint32_t poll_start_time_sec;     /* Start time of polling phase */
    uint8_t poll_iterations;          /* Iterations of GP1 polling completed */
    uint32_t poll_last_ms;            /* Last poll time in ms (for interval tracking) */
} s_sleep_state = {
    .state = SLEEP_STATE_ACTIVE,
    .wake_reason = WAKE_REASON_UNKNOWN,
    .inactivity_timer_sec = 0,
    .countdown_timer_sec = 0,
    .poll_start_time_sec = 0,
    .poll_iterations = 0,
    .poll_last_ms = 0,
};

/* Forward declarations */
static esp_err_t sleep_enter_light_sleep(void);
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
            
            /* Poll R503 interrupt (GP1) during countdown for user activity */
            if (read_r503_interrupt_pin()) {
                ESP_LOGI(TAG, "R503 interrupt detected during countdown, resetting timers");
                sleep_service_activity_detected();
                return;
            }
        } else {
            /* Countdown finished: enter sleep */
            ESP_LOGI(TAG, "All sleep entry conditions met — entering sleep");
            s_sleep_state.state = SLEEP_STATE_PRE_SLEEP;
            
            esp_err_t ret = sleep_service_enter();
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "sleep_service_enter() failed: %s", esp_err_to_name(ret));
                /* Reset state on error */
                s_sleep_state.state = SLEEP_STATE_ACTIVE;
                s_sleep_state.inactivity_timer_sec = 0;
            }
        }
    }
    else if (s_sleep_state.state == SLEEP_STATE_LIGHT_SLEEP) {
        /* During light sleep polling phase: check R503 interrupt */
        if (sleep_service_poll_r503()) {
            /* R503 interrupt detected! Wake up fully */
            ESP_LOGI(TAG, "R503 interrupt detected during sleep polling, waking up");
            sleep_service_wake();
        }
    }
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
        ESP_LOGE(TAG, "ERROR: Cannot enter sleep — state is %d (expected PRE_SLEEP)",
                 s_sleep_state.state);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Turning off LCD backlight");
    /* Turn off LCD backlight via CH422G */
    ch422g_backlight_control(I2C_MASTER_NUM, false);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    /* Enter light sleep */
    ESP_LOGI(TAG, "Entering light sleep with RTC wake every %" PRIu32 "s", (uint32_t)SLEEP_RTC_INTERVAL_SEC);
    esp_err_t ret = sleep_enter_light_sleep();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter light sleep: %s", esp_err_to_name(ret));
        /* Restore backlight on failure */
        ch422g_backlight_control(I2C_MASTER_NUM, true);
        s_sleep_state.state = SLEEP_STATE_ACTIVE;
        return ret;
    }
    
    /* Light sleep entered — execution resumes here on RTC wake */
    ESP_LOGI(TAG, "Woke from light sleep, polling R503 interrupt");
    
    /* Move to light sleep state and start polling */
    s_sleep_state.state = SLEEP_STATE_LIGHT_SLEEP;
    s_sleep_state.poll_start_time_sec = 0;
    s_sleep_state.poll_iterations = 0;
    s_sleep_state.poll_last_ms = 0;
    
    return ESP_OK;
}

/**
 * @brief Poll SC16IS752 GP1 for R503 interrupt during light sleep wake cycle
 *
 * Called from sleep_service_tick() during light sleep state.
 * Polls GP1 every SLEEP_POLL_INTERVAL_MS (500ms) for SLEEP_POLL_DURATION_SEC (10s).
 * If LOW detected (R503 touched) → fully wake up.
 * If HIGH for entire 10s → return to light sleep.
 */
bool sleep_service_poll_r503(void)
{
    if (s_sleep_state.state != SLEEP_STATE_LIGHT_SLEEP) {
        ESP_LOGD(TAG, "poll_r503: not in LIGHT_SLEEP state, skipping");
        return false;
    }
    
    /* On first call from wake, start timers */
    if (s_sleep_state.poll_iterations == 0) {
        s_sleep_state.poll_start_time_sec = (uint32_t)(esp_timer_get_time() / 1000000);
        s_sleep_state.poll_last_ms = (uint32_t)(esp_timer_get_time() / 1000);  /* Track last poll time in ms */
        ESP_LOGI(TAG, "Starting 10-second R503 polling phase (every 500ms, max 20 polls)");
    }
    
    /* Check absolute poll timing */
    uint32_t current_time_sec = (uint32_t)(esp_timer_get_time() / 1000000);
    uint32_t current_time_ms = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t elapsed = current_time_sec - s_sleep_state.poll_start_time_sec;
    
    /* Check current time */
    ESP_LOGD(TAG, "poll_r503: iteration %d, elapsed=%" PRIu32 "s (current=%" PRIu32 "s, start=%" PRIu32 "s)", 
             s_sleep_state.poll_iterations, elapsed, current_time_sec, s_sleep_state.poll_start_time_sec);
    
    if (elapsed >= SLEEP_POLL_DURATION_SEC) {
        /* Polling phase complete: no R503 activity, back to sleep */
        ESP_LOGI(TAG, "Polling phase complete (%" PRIu32 "s), no R503 activity detected, returning to sleep", elapsed);
        s_sleep_state.poll_iterations = 0;
        
        /* Return to light sleep */
        esp_err_t ret = sleep_enter_light_sleep();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to re-enter light sleep: %s", esp_err_to_name(ret));
        }
        return false;
    }
    
    /* Check time since last poll to apply poll interval */
    uint32_t ms_since_last = current_time_ms - s_sleep_state.poll_last_ms;
    
    /* Poll GP1 at configured interval */
    if (ms_since_last >= SLEEP_POLL_INTERVAL_MS) {
        s_sleep_state.poll_last_ms = current_time_ms;   /* Update last poll time */
        
        if (read_r503_interrupt_pin()) {
            /* R503 interrupt detected! Wake up. */
            ESP_LOGI(TAG, "R503 interrupt detected after %" PRIu32 "s polling, waking system", elapsed);
            s_sleep_state.wake_reason = WAKE_REASON_R503_INTERRUPT;
            s_sleep_state.poll_iterations = 0;
            return true;
        }
        s_sleep_state.poll_iterations++;
    }
    
    return false;
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
    s_sleep_state.poll_start_time_sec = 0;
    s_sleep_state.poll_iterations = 0;
    s_sleep_state.poll_last_ms = 0;
    
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
    s_sleep_state.poll_start_time_sec = 0;
    s_sleep_state.poll_iterations = 0;
    s_sleep_state.poll_last_ms = 0;
}

/* ---- Internal helper functions ---- */

/**
 * @brief Enter light sleep with RTC wake configured
 *
 * Sets up RTC timer to wake every SLEEP_RTC_INTERVAL_SEC seconds,
 * then calls esp_light_sleep_start().
 */
static esp_err_t sleep_enter_light_sleep(void)
{
    /* Configure RTC timer to wake every SLEEP_RTC_INTERVAL_SEC seconds */
    esp_err_t ret = esp_sleep_enable_timer_wakeup(
        SLEEP_RTC_INTERVAL_SEC * 1000000ULL  /* Convert seconds to microseconds */
    );
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RTC timer: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "RTC timer configured for %" PRIu32 "s intervals", (uint32_t)SLEEP_RTC_INTERVAL_SEC);
    
    /* Enter light sleep */
    /* esp_light_sleep_start() returns when woken by RTC (or other wake source) */
    esp_light_sleep_start();
    
    return ESP_OK;
}

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
    uint8_t gpio_state = 0;
    esp_err_t ret = sc16is752_transport_gpio_read(&gpio_state);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read SC16IS752 GPIO: %s", esp_err_to_name(ret));
        return false;
    }
    
    /* GP1 is bit 1 of gpio_state */
    bool gp1_state = (gpio_state & 0x02) == 0;  /* LOW when bit 1 is 0 */
    
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

bool sleep_service_poll_r503(void)
{
    return false;
}

#endif /* CONFIG_LOCKBOX_FEATURE_SLEEP_MODE */
