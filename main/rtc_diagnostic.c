#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "devices/ds3231_rtc.h"
#include "i2c_bus.h"

static const char *TAG = "RTC_DIAGNOSTIC";

/**
 * Diagnostic test to isolate RTC functionality from SD/CH422G operations
 * This runs after the UI is initialized but before normal authentication flow
 * Enable via CONFIG_LOCKBOX_RTC_DIAGNOSTIC in menuconfig
 */
void rtc_diagnostic_test(void) {
    ESP_LOGI(TAG, "=== RTC DIAGNOSTIC TEST START ===");
    
    // Wait 5 seconds to allow system to stabilize
    ESP_LOGI(TAG, "Waiting 5 seconds for system to stabilize...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Test 1: RTC read without any SD operations
    ESP_LOGI(TAG, "Test 1: Read RTC date (no SD operations)");
    i2c_bus_lock("RTC_DIAGNOSTIC_1");
    
    ds3231_date_t date_data;
    esp_err_t ret = ds3231_rtc_get_date(&date_data);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ RTC date read SUCCESS: %02d-%02d-%04d",
                 date_data.day, date_data.month, date_data.year);
    } else {
        ESP_LOGE(TAG, "✗ RTC date read FAILED: 0x%x", ret);
    }
    
    i2c_bus_unlock("RTC_DIAGNOSTIC_1");
    
    // Wait before next test
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Test 2: RTC read after waiting (no intervening ops)
    ESP_LOGI(TAG, "Test 2: Read RTC date again (after 2s wait)");
    i2c_bus_lock("RTC_DIAGNOSTIC_2");
    
    ret = ds3231_rtc_get_date(&date_data);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ RTC date read SUCCESS: %02d-%02d-%04d",
                 date_data.day, date_data.month, date_data.year);
    } else {
        ESP_LOGE(TAG, "✗ RTC date read FAILED: 0x%x", ret);
    }
    
    i2c_bus_unlock("RTC_DIAGNOSTIC_2");
    
    // Test 3: Rapid consecutive reads
    ESP_LOGI(TAG, "Test 3: Rapid consecutive RTC reads");
    for (int i = 0; i < 5; i++) {
        i2c_bus_lock("RTC_DIAGNOSTIC_3");
        
        ret = ds3231_rtc_get_date(&date_data);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  Read %d: ✓ %02d-%02d-%04d",
                     i+1, date_data.day, date_data.month, date_data.year);
        } else {
            ESP_LOGE(TAG, "  Read %d: ✗ FAILED (0x%x)", i+1, ret);
        }
        
        i2c_bus_unlock("RTC_DIAGNOSTIC_3");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    ESP_LOGI(TAG, "=== RTC DIAGNOSTIC TEST END ===");
}
