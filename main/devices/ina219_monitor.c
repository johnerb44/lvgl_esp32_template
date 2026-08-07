/**
 * @file ina219_monitor.c
 * @brief INA219 power monitor implementation using legacy ESP-IDF I2C driver
 *
 * Uses the same legacy ESP-IDF I2C driver (i2c_param_config + i2c_driver_install)
 * as waveshare_lcd_port, SC16IS752, and PCA9685 to coexist on the shared bus.
 *
 * Based on ina219.md design document
 */

#include "ina219_monitor.h"
#include "esp_log.h"
#include "esp_err.h"
#include "nvs.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include <math.h>
#include <string.h>

// ==================== INA219 Register Addresses ====================
#define INA219_REG_CONFIG       0x00
#define INA219_REG_SHUNT_VOLT   0x01
#define INA219_REG_BUS_VOLT     0x02
#define INA219_REG_POWER        0x03
#define INA219_REG_CURRENT      0x04
#define INA219_REG_CALIBRATION  0x05

// Config register bit positions
#define CONFIG_BRNG_BIT         13  // Bus voltage range
#define CONFIG_PG_BIT           11  // Gain
#define CONFIG_BADC_BIT         7   // Bus ADC resolution
#define CONFIG_SADC_BIT         3   // Shunt ADC resolution
#define CONFIG_MODE_BIT         0   // Operating mode

// Config register values
#define CONFIG_BRNG_16V         0x00
#define CONFIG_PG_0_32V         0x00
#define CONFIG_BADC_12BIT_1S    0x07
#define CONFIG_SADC_12BIT_1S    0x07
#define CONFIG_MODE_CONT        0x07

// ==================== Power Monitoring Constants ====================
#define INA219_DEFAULT_SHUNT_MILLIOHM   100
#define INA219_I2C_FREQ_HZ              400000  // 400kHz — max for new I2C driver in ESP-IDF 5.x
#define INA219_NS                       "powermon"
#define INA219_SAMPLE_MS                1000
#define INA219_SAVE_PERIOD_MS           60000
#define INA219_SAVE_DELTA_WH            0.20f
#define INA219_DEFAULT_FULL_WH          74.0f
#define INA219_ALPHA                    0.20f
#define INA219_MAX_SENSOR_FAILS         5

// I2C pins (same as waveshare board)
#define INA219_I2C_MASTER_SDA_IO        8
#define INA219_I2C_MASTER_SCL_IO        9

// SOC thresholds (with hysteresis)
#define INA219_T_HIGH                   60.0f
#define INA219_T_LOW                    25.0f
#define INA219_HYST                     5.0f

static const char *TAG = "INA219_MONITOR";
static i2c_port_t s_i2c_port = 0;
static uint8_t s_i2c_addr = 0x41;
static bool s_initialized = false;

// Power monitoring state
static float s_full_usable_wh = INA219_DEFAULT_FULL_WH;
static float s_used_wh = 0.0f;
static float s_used_mah_5v = 0.0f;
static int64_t s_last_save_us = 0;
static float s_used_wh_at_last_save = 0.0f;
static int64_t s_last_sample_us = 0;

// Filtering state
static bool s_filter_initialized = false;
static float s_voltage_f_v = 0.0f;
static float s_current_f_ma = 0.0f;

// Calibration constants
static float s_i_lsb = 0.01f;   // Current LSB in mA per count
static float s_p_lsb = 0.2f;    // Power LSB in mW per count

// Shunt resistor value
static float s_shunt_r_ohm = INA219_DEFAULT_SHUNT_MILLIOHM / 1000.0f;

// Cached power data
static ina219_power_data_t s_cached_power = {0};

// ==================== I2C Read/Write Helpers ====================

static esp_err_t ina219_i2c_read16(uint8_t reg, uint16_t *val)
{
    uint8_t reg_addr = reg;
    uint8_t data[2];

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) return ESP_ERR_NO_MEM;

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (s_i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (s_i2c_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 2, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
        // INA219 returns registers in big-endian, convert to CPU order
        *val = ((uint16_t)data[0] << 8) | data[1];
    }
    return ret;
}

static esp_err_t ina219_i2c_write16(uint8_t reg, uint16_t val)
{
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (val >> 8) & 0xFF;  // Big-endian for INA219
    buf[2] = val & 0xFF;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) return ESP_ERR_NO_MEM;

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (s_i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, buf, 3, true);
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return ret;
}

// ==================== NVS Persistence ====================

static esp_err_t nvs_load(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(INA219_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open (read) failed: %s, using defaults", esp_err_to_name(err));
        s_used_wh = 0.0f;
        s_used_mah_5v = 0.0f;
        s_full_usable_wh = INA219_DEFAULT_FULL_WH;
        return ESP_OK;
    }

    float f = 0.0f;
    err = nvs_get_blob(h, "used_wh", &f, &(size_t){sizeof(float)});
    s_used_wh = (err == ESP_OK) ? f : 0.0f;

    err = nvs_get_blob(h, "used_mah", &f, &(size_t){sizeof(float)});
    s_used_mah_5v = (err == ESP_OK) ? f : 0.0f;

    err = nvs_get_blob(h, "full_wh", &f, &(size_t){sizeof(float)});
    s_full_usable_wh = (err == ESP_OK && f > 1.0f && f < 200.0f) ? f : INA219_DEFAULT_FULL_WH;

    nvs_close(h);
    return ESP_OK;
}

static esp_err_t nvs_save(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(INA219_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(h, "used_wh", &s_used_wh, sizeof(float));
    if (err == ESP_OK) err = nvs_set_blob(h, "used_mah", &s_used_mah_5v, sizeof(float));
    if (err == ESP_OK) err = nvs_set_blob(h, "full_wh", &s_full_usable_wh, sizeof(float));
    if (err == ESP_OK) err = nvs_commit(h);

    nvs_close(h);
    return err;
}

// ==================== Level State Machine (Hysteresis) ====================

static uint8_t level_next(uint8_t current, float soc_pct)
{
    if (soc_pct < 0.0f) soc_pct = 0.0f;
    if (soc_pct > 100.0f) soc_pct = 100.0f;

    // Unknown -> choose nearest nominal band
    if (current == 0) {
        if (soc_pct >= INA219_T_HIGH) return 3;  // HIGH
        if (soc_pct < INA219_T_LOW) return 1;     // LOW
        return 2;  // MEDIUM
    }

    // HIGH -> MEDIUM below 55
    if (current == 3) {
        if (soc_pct < (INA219_T_HIGH - INA219_HYST)) return 2;
        return 3;
    }

    // MEDIUM -> HIGH above 65, MEDIUM -> LOW below 20
    if (current == 2) {
        if (soc_pct > (INA219_T_HIGH + INA219_HYST)) return 3;
        if (soc_pct < (INA219_T_LOW - INA219_HYST)) return 1;
        return 2;
    }

    // LOW -> MEDIUM above 30
    if (current == 1) {
        if (soc_pct > (INA219_T_LOW + INA219_HYST)) return 2;
        return 1;
    }

    return 0;
}

// ==================== Core Update Logic ====================

static void update_from_sample(float v_bus, float i_ma, float dt_h)
{
    // Clamp discharge-only
    if (i_ma < 0.0f) i_ma = 0.0f;
    if (v_bus < 0.0f) v_bus = 0.0f;

    // Exponential moving average filtering
    if (!s_filter_initialized) {
        s_voltage_f_v = v_bus;
        s_current_f_ma = i_ma;
        s_filter_initialized = true;
    } else {
        s_voltage_f_v = INA219_ALPHA * v_bus + (1.0f - INA219_ALPHA) * s_voltage_f_v;
        s_current_f_ma = INA219_ALPHA * i_ma  + (1.0f - INA219_ALPHA) * s_current_f_ma;
    }

    float power_w = (s_voltage_f_v * s_current_f_ma) / 1000.0f;
    float d_mah = s_current_f_ma * dt_h;
    float d_wh  = power_w * dt_h;

    s_used_mah_5v += d_mah;
    s_used_wh += d_wh;
    if (s_used_wh < 0.0f) s_used_wh = 0.0f;
    if (s_full_usable_wh < 1.0f) s_full_usable_wh = INA219_DEFAULT_FULL_WH;

    float remaining_wh = s_full_usable_wh - s_used_wh;
    if (remaining_wh < 0.0f) remaining_wh = 0.0f;

    float soc_pct = 100.0f * (remaining_wh / s_full_usable_wh);
    if (soc_pct < 0.0f) soc_pct = 0.0f;
    if (soc_pct > 100.0f) soc_pct = 100.0f;

    // Update cached data
    s_cached_power.voltage_v = s_voltage_f_v;
    s_cached_power.current_ma = s_current_f_ma;
    s_cached_power.power_w = power_w;
    s_cached_power.used_wh = s_used_wh;
    s_cached_power.remaining_wh = remaining_wh;
    s_cached_power.soc_pct = soc_pct;

    s_cached_power.level = level_next(s_cached_power.level, soc_pct);
    s_cached_power.sensor_ok = true;
}

static bool should_save(int64_t now_us)
{
    bool periodic = (now_us - s_last_save_us) >= ((int64_t)INA219_SAVE_PERIOD_MS * 1000);
    bool delta_wh = fabsf(s_used_wh - s_used_wh_at_last_save) >= INA219_SAVE_DELTA_WH;
    return periodic || delta_wh;
}

// ==================== Calibration ====================

static void calibrate_internal(float r_shunt_ohm)
{
    // Gain 0.32V → max shunt voltage = 0.32V
    // I_LSB = (0.32 * 1000000) / (shunt_mOhm * 32767)
    float i_lsb_raw = (0.32f * 1000000.0f) / (r_shunt_ohm * 1000.0f * 32767.0f);
    i_lsb_raw = ceilf(i_lsb_raw / 0.0001f) * 0.0001f;
    s_i_lsb = i_lsb_raw;
    s_p_lsb = s_i_lsb * 20.0f;

    uint16_t cal = (uint16_t)((0.04096f) / (s_i_lsb * r_shunt_ohm));
    ESP_LOGI(TAG, "INA219 calibrated: shunt=%.1fmOhm, I_LSB=%.4f mA/count, CAL=0x%04X",
             r_shunt_ohm * 1000.0f, s_i_lsb, cal);
}

// ==================== Public API ====================

esp_err_t ina219_monitor_init(int i2c_port_num, uint8_t i2c_address)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE && CONFIG_LOCKBOX_FEATURE_INA219
    if (s_initialized) {
        ESP_LOGW(TAG, "INA219 monitor already initialized");
        return ESP_OK;
    }

    s_i2c_port = i2c_port_num;
    s_i2c_addr = i2c_address;
    s_cached_power.level = 0;
    s_cached_power.sensor_ok = false;

    ESP_LOGI(TAG, "Initializing INA219 on I2C port %d, address 0x%02X", s_i2c_port, s_i2c_addr);

    // Use legacy ESP-IDF I2C driver (same bus as waveshare/SC16IS752/PCA9685)
    // Configure the bus for INA219 speed (1 MHz)
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = INA219_I2C_MASTER_SDA_IO,
        .scl_io_num = INA219_I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = INA219_I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(s_i2c_port, &conf);
    if (err == ESP_ERR_INVALID_STATE || err == ESP_ERR_INVALID_ARG) {
        // Port already configured (by waveshare/SC16IS752/PCA9685), reuse
        ESP_LOGI(TAG, "I2C port %d already configured, reusing existing bus", s_i2c_port);
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }

    // Verify device is present by reading config register
    uint16_t config_reg = 0;
    err = ina219_i2c_read16(INA219_REG_CONFIG, &config_reg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read INA219 config register: %s", esp_err_to_name(err));
        ESP_LOGW(TAG, "INA219 may not be present on I2C bus");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "INA219 present, config register = 0x%04X", config_reg);

    // Configure INA219: 16V range, 0.32V gain, 12-bit, continuous mode
    uint16_t config = (CONFIG_BRNG_16V << CONFIG_BRNG_BIT) |
                      (CONFIG_PG_0_32V << CONFIG_PG_BIT) |
                      (CONFIG_BADC_12BIT_1S << CONFIG_BADC_BIT) |
                      (CONFIG_SADC_12BIT_1S << CONFIG_SADC_BIT) |
                      (CONFIG_MODE_CONT << CONFIG_MODE_BIT);

    err = ina219_i2c_write16(INA219_REG_CONFIG, config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure INA219: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "INA219 configured: 0x%04X", config);

    // Calibrate with shunt resistor
    calibrate_internal(s_shunt_r_ohm);

    // Load NVS data
    nvs_load();

    s_last_sample_us = esp_timer_get_time();
    s_last_save_us = s_last_sample_us;
    s_used_wh_at_last_save = s_used_wh;

    s_initialized = true;
    ESP_LOGI(TAG, "INA219 monitor initialized successfully");
    return ESP_OK;
#else
    ESP_LOGI(TAG, "INA219 monitor disabled (LOCKBOX_INTEGRATION or INA219 not enabled)");
    return ESP_OK;
#endif
}

esp_err_t ina219_monitor_deinit(void)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE && CONFIG_LOCKBOX_FEATURE_INA219
    s_initialized = false;
    memset(&s_cached_power, 0, sizeof(s_cached_power));
    ESP_LOGI(TAG, "INA219 monitor deinitialized");
    return ESP_OK;
#else
    return ESP_OK;
#endif
}

esp_err_t ina219_monitor_read_power_data(ina219_power_data_t *power_data)
{
#if CONFIG_LOCKBOX_INTEGRATION_ENABLE && CONFIG_LOCKBOX_FEATURE_INA219
    if (!s_initialized) {
        ESP_LOGE(TAG, "INA219 monitor not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    if (power_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int64_t now_us = esp_timer_get_time();
    float dt_h = (float)(now_us - s_last_sample_us) / 1000000.0f / 3600.0f;
    s_last_sample_us = now_us;

    // Read INA219 registers
    uint16_t bus_raw = 0, shunt_raw = 0;
    esp_err_t err = ina219_i2c_read16(INA219_REG_BUS_VOLT, &bus_raw);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read bus voltage: %s", esp_err_to_name(err));
        s_cached_power.sensor_ok = false;
        memcpy(power_data, &s_cached_power, sizeof(ina219_power_data_t));
        return err;
    }

    err = ina219_i2c_read16(INA219_REG_SHUNT_VOLT, &shunt_raw);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read shunt voltage: %s", esp_err_to_name(err));
        s_cached_power.sensor_ok = false;
        memcpy(power_data, &s_cached_power, sizeof(ina219_power_data_t));
        return err;
    }

    // Convert raw values to physical quantities
    float v_bus = bus_raw * 0.004f;   // 4mV per LSB
    int16_t shunt_mv = (int16_t)(shunt_raw >> 3);  // 10-bit two's complement, mV
    float i_ma = shunt_mv / (s_shunt_r_ohm * 1000.0f) * 1000.0f;

    // Validate sensor health
    if (v_bus < 1.0f || v_bus > 20.0f) {
        static int fail_count = 0;
        if (++fail_count > INA219_MAX_SENSOR_FAILS) {
            s_cached_power.sensor_ok = false;
        }
        ESP_LOGW(TAG, "INA219 voltage out of range: %.3fV", v_bus);
    } else {
        s_cached_power.sensor_ok = true;
    }

    // Update calculations
    update_from_sample(v_bus, i_ma, dt_h);

    // Copy to output
    memcpy(power_data, &s_cached_power, sizeof(ina219_power_data_t));

    // Save if needed
    if (should_save(now_us)) {
        err = nvs_save();
        if (err == ESP_OK) {
            s_last_save_us = now_us;
            s_used_wh_at_last_save = s_used_wh;
        }
    }

    return ESP_OK;
#else
    if (power_data) {
        power_data->voltage_v = 5.0f;
        power_data->current_ma = 0.0f;
        power_data->power_w = 0.0f;
        power_data->used_wh = 0.0f;
        power_data->remaining_wh = 74.0f;
        power_data->soc_pct = 100.0f;
        power_data->level = 3;
        power_data->sensor_ok = false;
    }
    return ESP_OK;
#endif
}

esp_err_t ina219_monitor_set_full_capacity(float full_wh)
{
    if (full_wh <= 0.0f || full_wh > 200.0f) {
        return ESP_ERR_INVALID_ARG;
    }
    s_full_usable_wh = full_wh;
    return ESP_OK;
}

esp_err_t ina219_monitor_mark_full(void)
{
    s_used_wh = 0.0f;
    s_used_mah_5v = 0.0f;
    s_cached_power.level = 3;
    s_cached_power.remaining_wh = s_full_usable_wh;
    s_cached_power.soc_pct = 100.0f;
    s_cached_power.sensor_ok = true;

    esp_err_t err = nvs_save();
    if (err == ESP_OK) {
        s_last_save_us = esp_timer_get_time();
        s_used_wh_at_last_save = s_used_wh;
    }
    return err;
}

esp_err_t ina219_monitor_get_status(ina219_power_data_t *power_data)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (power_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(power_data, &s_cached_power, sizeof(ina219_power_data_t));
    return ESP_OK;
}
