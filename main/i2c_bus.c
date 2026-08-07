/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Bus Mutual Exclusion Layer Implementation
 */

#include "i2c_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char *I2C_BUS_TAG = "I2C_BUS";

/**
 * @brief Global I2C bus mutex (recursive, to allow nested locking).
 *        Protects access to I2C_NUM_0 across all devices.
 */
static SemaphoreHandle_t i2c_bus_mutex = NULL;

esp_err_t i2c_bus_init(void)
{
    if (i2c_bus_mutex != NULL) {
        ESP_LOGW(I2C_BUS_TAG, "I2C bus mutex already initialized (double init ignored)");
        return ESP_OK;
    }

    // Use recursive mutex to allow the same task to acquire the lock multiple times
    i2c_bus_mutex = xSemaphoreCreateRecursiveMutex();
    if (i2c_bus_mutex == NULL) {
        ESP_LOGE(I2C_BUS_TAG, "Failed to create I2C bus recursive mutex");
        return ESP_FAIL;
    }

    ESP_LOGI(I2C_BUS_TAG, "I2C bus recursive mutex initialized (timeout=%d ms)", I2C_BUS_LOCK_TIMEOUT_MS);
    return ESP_OK;
}

void i2c_bus_lock(const char *owner)
{
    if (i2c_bus_mutex == NULL) {
        ESP_LOGE(I2C_BUS_TAG, "I2C bus mutex not initialized! (called by %s) — deadlock risk!", owner);
        return;
    }

    ESP_LOGD(I2C_BUS_TAG, "I2C_BUS_LOCK requested by %s (timeout=%d ms)", owner, I2C_BUS_LOCK_TIMEOUT_MS);

    BaseType_t ret = xSemaphoreTakeRecursive(i2c_bus_mutex, pdMS_TO_TICKS(I2C_BUS_LOCK_TIMEOUT_MS));
    if (ret != pdTRUE) {
        ESP_LOGE(I2C_BUS_TAG, "I2C_BUS_LOCK TIMEOUT from %s (held >%d ms) — deadlock or high contention!", owner, I2C_BUS_LOCK_TIMEOUT_MS);
        return;
    }

    ESP_LOGD(I2C_BUS_TAG, "I2C_BUS_LOCK acquired by %s", owner);
}

void i2c_bus_unlock(const char *owner)
{
    if (i2c_bus_mutex == NULL) {
        ESP_LOGE(I2C_BUS_TAG, "I2C bus mutex not initialized! (called by %s) — orphaned unlock!", owner);
        return;
    }

    ESP_LOGD(I2C_BUS_TAG, "I2C_BUS_UNLOCK releasing from %s", owner);
    xSemaphoreGiveRecursive(i2c_bus_mutex);
}
