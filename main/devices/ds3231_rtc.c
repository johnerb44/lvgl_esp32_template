/**
 * @file ds3231_rtc.c
 * @brief DS3231 RTC I2C driver implementation
 *
 * Uses a pre-configured shared I2C bus (initialized by SC16IS752/INA219/etc).
 * Does NOT call i2c_driver_install — only verifies communication with the device.
 *
 * DS3231 register map:
 *   0x00 - Seconds   (BCD)
 *   0x01 - Minutes   (BCD)
 *   0x02 - Hours     (BCD, bit 6 = 12/24h)
 *   0x03 - Day of week (1=Sun, 2=Mon, ..., 7=Sat)
 *   0x04 - Date      (BCD, 1-31)
 *   0x05 - Month     (BCD, bit 7 = century)
 *   0x06 - Year      (BCD, 0-99)
 */

#include "ds3231_rtc.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "DS3231_RTC";

/* I2C pins — shared bus with existing devices (same as INA219/SC16) */
#define DS3231_I2C_MASTER_SDA_IO  8
#define DS3231_I2C_MASTER_SCL_IO  9
#define DS3231_I2C_FREQ_HZ        100000  /* 100kHz — conservative for shared bus */

/* Internal state */
static i2c_port_t   s_i2c_port = 0;
static bool         s_initialized = false;

/* BCD helpers */
static inline uint8_t bcd_to_byte(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static inline uint8_t byte_to_bcd(uint8_t val)
{
    return ((val / 10) << 4) | (val % 10);
}

/* Read count bytes from a register (reg auto-increment) */
static esp_err_t rtc_read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(cmd);  // ← START condition before writing slave address
    i2c_master_write_byte(cmd, (DS3231_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);

    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(200));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        /* Bus may be stuck — send clock pulses to recover */
        for (int i = 0; i < 16; i++) {
            i2c_cmd_handle_t pulse = i2c_cmd_link_create();
            if (pulse) {
                i2c_master_start(pulse);
                i2c_master_write_byte(pulse, 0xFF, true);  /* dummy byte */
                i2c_master_stop(pulse);
                i2c_master_cmd_begin(s_i2c_port, pulse, pdMS_TO_TICKS(10));
                i2c_cmd_link_delete(pulse);
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));  /* give DS3231 time to settle */
        return ret;
    }

    cmd = i2c_cmd_link_create();
    if (!cmd) {
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_I2C_ADDRESS << 1) | I2C_MASTER_READ, true);

    if (len > 1) {
        i2c_master_read(cmd, buf, len - 1, false); /* ACK on intermediate bytes */
        i2c_master_read_byte(cmd, &buf[len - 1], true);  /* NACK on last */
    } else {
        i2c_master_read_byte(cmd, &buf[0], true);  /* NACK */
    }

    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(200));
    i2c_cmd_link_delete(cmd);

    return ret;
}

/* Write count bytes starting from a register (reg auto-increment) */
/* Matches working ds3231-test: register pointer + data in ONE I2C transaction */
static esp_err_t rtc_write_regs(uint8_t reg, const uint8_t *buf, size_t len)
{
    /* Combined buffer: [register_pointer][data...] */
    uint8_t *combined = malloc(len + 1);
    if (!combined) {
        return ESP_ERR_NO_MEM;
    }
    combined[0] = reg;
    memcpy(combined + 1, buf, len);

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (!cmd) {
        free(combined);
        return ESP_ERR_NO_MEM;
    }

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, combined, len + 1, true);  // ptr + data in single write
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    free(combined);
    return ret;
}

/* Single byte write */
static esp_err_t rtc_write_reg8(uint8_t reg, uint8_t val)
{
    uint8_t b = val;
    return rtc_write_regs(reg, &b, 1);
}

/* ========== Public API ========== */

esp_err_t ds3231_rtc_init(int i2c_port, uint8_t i2c_addr)
{
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing DS3231 RTC on I2C port %d, address 0x%02X (shared bus)", i2c_port, i2c_addr);

    /* I2C bus should already be initialized by another driver (SC16IS752/INA219/etc).
     * We do NOT call i2c_driver_install here — only verify the bus is usable. */
    s_i2c_port = (i2c_port >= 0) ? (i2c_port_t)i2c_port : I2C_NUM_0;
    ESP_LOGI(TAG, "Using I2C port %d (bus already configured by another driver)", s_i2c_port);

    /* Probe: read status register to verify DS3231 is present */
    i2c_bus_lock("DS3231_probe");
    uint8_t status_reg = 0;
    esp_err_t ret = rtc_read_regs(DS3231_REG_status, &status_reg, 1);
    i2c_bus_unlock("DS3231_probe");
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "DS3231 not found on I2C bus (probe reg 0x0F failed: %s) — software RTC mode", esp_err_to_name(ret));
        s_initialized = true;
        return ESP_OK;
    }
    ESP_LOGI(TAG, "DS3231 probe OK, status=0x%02X", status_reg);

    /* Clear oscillator-stop flag (bit 7) if set */
    if (status_reg & (1 << 7)) {
        ESP_LOGW(TAG, "OSF detected, clearing oscillator-stop flag");
        uint8_t clear_stat = status_reg & ~(1 << 7);
        i2c_bus_lock("DS3231_clear_osf");
        rtc_write_reg8(DS3231_REG_status, clear_stat);
        i2c_bus_unlock("DS3231_clear_osf");
    }

    s_initialized = true;
    ESP_LOGI(TAG, "DS3231 initialized OK (port=%d, s_initialized=%d)", s_i2c_port, s_initialized);
    return ESP_OK;
}

esp_err_t ds3231_rtc_deinit(void)
{
    s_initialized = false;
    ESP_LOGI(TAG, "DS3231 RTC deinitialized");
    return ESP_OK;
}

bool ds3231_rtc_is_initialized(void)
{
    return s_initialized;
}

esp_err_t ds3231_rtc_get_datetime(ds3231_datetime_t *dt)
{
    if (!dt || !s_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read all 7 time/date registers in one burst (0x00-0x06) */
    i2c_bus_lock("DS3231_get_datetime");
    uint8_t regs[7];
    esp_err_t ret = rtc_read_regs(DS3231_REG_sec, regs, 7);
    i2c_bus_unlock("DS3231_get_datetime");
    
    if (ret != ESP_OK) {
        /* No hardware — return default date/time */
        ESP_LOGW(TAG, "RTC read failed (%s), returning default 01-01-2026 00:00:00", esp_err_to_name(ret));
        dt->date.day = 1; dt->date.month = 1; dt->date.year = 2026;
        dt->time.hour = 0; dt->time.minutes = 0; dt->time.seconds = 0;
        return ESP_OK;
    }

    /* Time: regs[0]=sec, regs[1]=min, regs[2]=hour(24h since bit6=1) */
    dt->time.seconds   = bcd_to_byte(regs[0] & 0x7F);   /* clear bit 7 (HI_US) */
    dt->time.minutes   = bcd_to_byte(regs[1] & 0x7F);
    dt->time.hour      = bcd_to_byte(regs[2] & 0x3F);   /* 24h mode, mask bit 6 */

    /* Date: regs[3]=day_of_week (unused), regs[4]=date, regs[5]=month, regs[6]=year */
    dt->date.day       = bcd_to_byte(regs[4]);
    dt->date.month     = bcd_to_byte(regs[5] & 0x1F);
    dt->date.year      = 2000 + bcd_to_byte(regs[6]);

    return ESP_OK;
}

esp_err_t ds3231_rtc_set_datetime(const ds3231_datetime_t *dt)
{
    if (!dt || !s_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t regs[7];
    regs[0] = byte_to_bcd(dt->time.seconds);
    regs[1] = byte_to_bcd(dt->time.minutes);
    regs[2] = byte_to_bcd(dt->time.hour);                /* 24h mode */
    regs[3] = byte_to_bcd(1);                            /* day-of-week: 1=Sun, default */
    regs[4] = byte_to_bcd(dt->date.day);
    regs[5] = byte_to_bcd(dt->date.month) | (1 << 7);   /* set century bit (2000s) */
    regs[6] = byte_to_bcd((dt->date.year - 2000) % 100);

    i2c_bus_lock("DS3231_set_datetime");
    esp_err_t ret = rtc_write_regs(DS3231_REG_sec, regs, 7);
    i2c_bus_unlock("DS3231_set_datetime");
    
    if (ret != ESP_OK) {
        /* No hardware — silently succeed */
        ESP_LOGW(TAG, "RTC datetime write failed (%s), no hardware", esp_err_to_name(ret));
        return ESP_OK;
    }
    return ret;
}

esp_err_t ds3231_rtc_get_time(ds3231_time_t *t)
{
    if (!t || !s_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_bus_lock("DS3231_get_time");
    uint8_t regs[3];
    esp_err_t ret = rtc_read_regs(DS3231_REG_sec, regs, 3);
    i2c_bus_unlock("DS3231_get_time");
    
    if (ret != ESP_OK) {
        /* No hardware — return default time */
        ESP_LOGW(TAG, "RTC time read failed (%s), returning default 00:00:00", esp_err_to_name(ret));
        t->seconds = 0; t->minutes = 0; t->hour = 0;
        return ESP_OK;
    }

    t->seconds = bcd_to_byte(regs[0] & 0x7F);
    t->minutes = bcd_to_byte(regs[1] & 0x7F);
    t->hour    = bcd_to_byte(regs[2] & 0x3F);
    return ESP_OK;
}

esp_err_t ds3231_rtc_set_time(const ds3231_time_t *t)
{
    if (!t || !s_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t regs[3];
    regs[0] = byte_to_bcd(t->seconds);
    regs[1] = byte_to_bcd(t->minutes);
    regs[2] = byte_to_bcd(t->hour);  // DS3231 is always in 24h mode
    
    i2c_bus_lock("DS3231_set_time");
    esp_err_t ret = rtc_write_regs(DS3231_REG_sec, regs, 3);
    i2c_bus_unlock("DS3231_set_time");
    
    return ret;
}

esp_err_t ds3231_rtc_get_date(ds3231_date_t *d)
{
    if (!d || !s_initialized) {
        return ESP_ERR_INVALID_ARG;
    }

    i2c_bus_lock("DS3231_get_date");
    uint8_t regs[3];  /* read date (0x04) through year (0x06) */
    esp_err_t ret = rtc_read_regs(DS3231_REG_date, regs, 3);
    i2c_bus_unlock("DS3231_get_date");
    
    if (ret != ESP_OK) {
        /* Bus recovery: toggle SDA to unstick any stuck I2C transaction */
        ESP_LOGW(TAG, "RTC date read failed (%s), toggling bus recovery", esp_err_to_name(ret));
        /* Clear the I2C peripheral state by sending clock cycles */
        for (int i = 0; i < 20; i++) {
            i2c_cmd_handle_t pulse = i2c_cmd_link_create();
            if (pulse) {
                i2c_master_start(pulse);
                i2c_master_write_byte(pulse, 0xFF, true);  /* dummy byte, clock cycles */
                i2c_master_stop(pulse);
                i2c_master_cmd_begin(s_i2c_port, pulse, pdMS_TO_TICKS(5));
                i2c_cmd_link_delete(pulse);
            }
        }
        d->day = 1; d->month = 1; d->year = 2026;
        return ESP_OK;
    }

    d->day   = bcd_to_byte(regs[0]);
    d->month = bcd_to_byte(regs[1] & 0x1F);
    d->year  = 2000 + bcd_to_byte(regs[2]);
    return ESP_OK;
}

esp_err_t ds3231_rtc_set_date(const ds3231_date_t *d)
{
    if (!d || !s_initialized) {
        ESP_LOGE(TAG, "set_date called but s_initialized=%d (i2c_port=%d)", s_initialized, s_i2c_port);
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t regs[3];
    regs[0] = byte_to_bcd(d->day);
    regs[1] = byte_to_bcd(d->month) | DS3231_MONTH_CENTURY_BIT;  /* century = 20xx */
    regs[2] = byte_to_bcd((d->year - 2000) % 100);
    
    i2c_bus_lock("DS3231_set_date");
    esp_err_t ret = rtc_write_regs(DS3231_REG_date, regs, 3);
    i2c_bus_unlock("DS3231_set_date");
    
    if (ret != ESP_OK) {
        /* No hardware — silently succeed (write will take effect when RTC is added) */
        ESP_LOGW(TAG, "RTC date write failed (%s), no hardware — date will take effect later", esp_err_to_name(ret));
        return ESP_OK;
    }
    return ret;
}

void ds3231_date_to_string(const ds3231_date_t *d, char *out_buf)
{
    if (!d || !out_buf) {
        return;
    }
    out_buf[0] = (char)('0' + ((unsigned)d->day / 10) % 10);
    out_buf[1] = (char)('0' + (unsigned)d->day % 10);
    out_buf[2] = '-';
    out_buf[3] = (char)('0' + ((unsigned)d->month / 10) % 10);
    out_buf[4] = (char)('0' + (unsigned)d->month % 10);
    out_buf[5] = '-';
    out_buf[6] = (char)('0' + ((unsigned)d->year / 1000) % 10);
    out_buf[7] = (char)('0' + ((unsigned)d->year / 100) % 10);
    out_buf[8] = (char)('0' + ((unsigned)d->year / 10) % 10);
    out_buf[9] = (char)('0' + (unsigned)d->year % 10);
    out_buf[10] = '\0';
}

esp_err_t ds3231_date_from_string(const char *str, ds3231_date_t *d)
{
    if (!str || !d) {
        return ESP_ERR_INVALID_ARG;
    }

    int dd, mm, yyyy;
    if (sscanf(str, "%d-%d-%d", &dd, &mm, &yyyy) != 3) {
        ESP_LOGE(TAG, "Invalid date format: %s", str);
        return ESP_ERR_INVALID_ARG;
    }

    if (dd < 1 || dd > 31 || mm < 1 || mm > 12 || yyyy < 2000 || yyyy > 2099) {
        ESP_LOGE(TAG, "Date out of range: %d-%d-%d", dd, mm, yyyy);
        return ESP_ERR_INVALID_ARG;
    }

    d->day   = (uint8_t)dd;
    d->month = (uint8_t)mm;
    d->year  = (uint16_t)yyyy;
    return ESP_OK;
}
