# Battery voltage level monitoring

##  References

https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32s3/api-reference/peripherals/adc.html

https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-4.3?srsltid=AfmBOopZV1J7xzwDaC2KaSWZAzzpwvXq0r2TGIR-jDcKW8GblfyRVZ9o

/home/erbj/lvgl-projects/04_Sensor_AD

/home/erbj/lvgl-projects/lvgl_esp32_template/Lockbox-System.pdf

## Example discussion 

The Secure Lock Box system uses a 5 vdc NOBIS Portable Charger, 20000mAh Power Bank, 45W Power Bank Fast Charging, Battery Pack with C to C Cable, Battery Bank with Digital Display for iPhone Android Laptop etc. The model # is ZWPBWWA-202C2A45W 

To build a battery monitor, scale the battery's voltage (e.g., 4.2 V max for a LiPo) down to the ESP32-S3's ADC input range (under 1.1 V) using a resistor voltage divider. Use the ESP-IDF Oneshot ADC driver with ADC_ATTEN_DB_12 attenuation and built-in eFuse Curve-Fitting calibration to measure the voltage and compute the percentage.

Building an ESP32-S3 battery monitor requires a resistor voltage divider (to step down the battery voltage to safe ADC levels) and the ESP-IDF ADC API (for One-Shot readings and factory eFuse calibration).

1. Hardware Setup (Voltage Divider)Because the ESP32-S3's ADC can only measure up to 3.3 V, a single-cell Li-Po battery (4.2 V when fully charged) will damage the pin if connected directly.Connect R₁ (100 kΩ) from the Battery Positive to an ADC Pin (e.g., GPIO 4).Connect R₂ (100 kΩ) from the ADC Pin to GND.

2. This setup divides the voltage by 2, meaning a 4.2 V battery outputs 2.1 V to the pin, which is perfectly safe. Note: Ensure you connect the Battery GND to the ESP32 GND.
 
3. ESP-IDF Software Implementation:  This example uses the modern ESP-IDF ADC One-Shot driver (available in v5.x and later) which queries the chip's factory eFuse calibration parameters for accurate millivolt readings. Add this to your main/main.c file.

4. To build a battery monitor, scale the battery's voltage (e.g., 4.2 V max for a LiPo) down to the ESP32-S3's ADC input range (under 1.1 V) using a resistor voltage divider. Use the ESP-IDF Oneshot ADC driver with ADC_ATTEN_DB_12 attenuation and built-in eFuse Curve-Fitting calibration to measure the voltage and compute the percentage.

5. Calculating the Battery Percentage: Voltage curves for lithium batteries are non-linear. A simple approach is to map the voltages to a percentage using standard Min/Max thresholds (e.g., 3.2 V = 0% capacity, 5.0 V = 100% capacity) 

## Code Example 1

#include <stdio.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ADC_UNIT            ADC_UNIT_1
#define ADC_ATTEN           ADC_ATTEN_DB_12 // Allows measuring up to ~2.6V
#define ADC_CHANNEL         ADC_CHANNEL_3   // Corresponds to GPIO 4 (on most ESP32-S3 boards)

static const char *TAG = "BATTERY_MONITOR";

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing Battery Monitor...");

    // 1. ADC One-Shot Handle Configuration
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // 2. ADC Channel Configuration
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    // 3. ADC Calibration Handle (uses factory eFuse characteristics)
    adc_cali_handle_t cali_handle = NULL;
    bool cali_enable = false;

    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT,
            .atten = ADC_ATTEN,
            .bitwidth = ADC_BITWIDTH_12,
        };
        ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));
        cali_enable = true;
    #elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT,
            .atten = ADC_ATTEN,
            .bitwidth = ADC_BITWIDTH_12,
        };
        ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle));
        cali_enable = true;
    #endif

    int adc_raw = 0;
    int voltage_mv = 0;

    while (1) {
        // 4. Read Raw ADC value
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw));

        // 5. Convert Raw value to Millivolts
        if (cali_enable) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_mv));
        } else {
            voltage_mv = adc_raw * 1.1; // Fallback estimate
        }

        // 6. Calculate Real Battery Voltage (Compensate for the 1:2 Voltage Divider)
        int battery_voltage_mv = voltage_mv * 2; 

        ESP_LOGI(TAG, "Raw ADC: %d | ESP Pin Voltage: %d mV | Actual Battery Voltage: %d mV", 
                 adc_raw, voltage_mv, battery_voltage_mv);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

## Code Example 2 (main.c)

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_UNIT              ADC_UNIT_1
#define ADC_CHANNEL           ADC_CHANNEL_3  // Example: GPIO 4 on ESP32-S3
#define ADC_ATTEN             ADC_ATTEN_DB_12

// Voltage divider ratio: (R1 + R2) / R2
// e.g., R1 = 390k, R2 = 100k -> (390 + 100) / 100 = 4.9
#define VOLTAGE_DIVIDER_RATIO 4.9f 

static const char *TAG = "BAT_MONITOR";

static bool calibrate_adc(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    bool cali_hint_success = false;
    adc_cali_handle_t handle = NULL;

    #if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
        if (!cali_hint_success) {
            ESP_LOGI(TAG, "calibration scheme version is Curve Fitting");
            adc_cali_curve_fitting_config_t cali_config = {
                .unit_id = unit,
                .channel = channel,
                .atten = atten,
                .bitwidth = ADC_BITWIDTH_DEFAULT,
            };
            ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &handle));
            cali_hint_success = true;
        }
    #endif

    #if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
        if (!cali_hint_success) {
            ESP_LOGI(TAG, "calibration scheme version is Line Fitting");
            adc_cali_line_fitting_config_t cali_config = {
                .unit_id = unit,
                .channel = channel,
                .atten = atten,
                .bitwidth = ADC_BITWIDTH_DEFAULT,
            };
            ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_config, &handle));
            cali_hint_success = true;
        }
    #endif

    *out_handle = handle;
    if (cali_hint_success) {
        ESP_LOGI(TAG, "Calibration Success");
    } else {
        ESP_LOGE(TAG, "eFuse not burnt, skip software calibration");
    }

    return cali_hint_success;
}

void app_main(void) {
    // 1. ADC Unit Initialization
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    // 2. ADC Channel Configuration
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    // 3. Calibration Initialization
    adc_cali_handle_t cali_handle = NULL;
    bool do_calibration = calibrate_adc(ADC_UNIT, ADC_CHANNEL, ADC_ATTEN, &cali_handle);

    int adc_raw = 0;
    int voltage_mv = 0;

    while (1) {
        // 4. Read Raw ADC Value
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_raw));
        
        // 5. Convert using calibration or fallback to manual calculation
        if (do_calibration) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, adc_raw, &voltage_mv));
        } else {
            // Uncalibrated fallback, 12-bit (4095) with DB_12 atten (approx 1100mV)
            voltage_mv = (adc_raw * 1100) / 4095; 
        }

        // 6. Calculate actual battery voltage and percentage
        float bat_voltage = (float)voltage_mv * VOLTAGE_DIVIDER_RATIO / 1000.0f;
        
        // Example linear mapping for a 3.7V Li-Po (100% = 4.2V, 0% = 3.2V)
        float bat_percentage = 0.0f;
        if (bat_voltage > 4.2f) bat_percentage = 100.0f;
        else if (bat_voltage < 3.2f) bat_percentage = 0.0f;
        else bat_percentage = (bat_voltage - 3.2f) * 100.0f / (4.2f - 3.2f);

        ESP_LOGI(TAG, "Raw: %d, Divided mV: %d, Battery Voltage: %.2f V, SoC: %.1f%%", 
                 adc_raw, voltage_mv, bat_voltage, bat_percentage);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Deinitialize handles if exiting (Not reached in this loop)
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
}

Use code with caution.

Configuring CMakeLists.txt and idf.pyIn the ESP-IDF, the ADC components are separated. Ensure your project main/CMakeLists.txt properly references the ADC 

component:cmakeidf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_adc)

