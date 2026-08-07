#include "devices/pca9685_device.h"
#include <stdbool.h>
#include <stdlib.h>
#include "sd/sd_card.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "PCA9685_DEVICE";
static bool s_initialized = false;

#define PCA9685_I2C_ADDR            0x40
#define PCA9685_MODE1_REG           0x00
#define PCA9685_PRESCALE_REG        0xFE
#define PCA9685_LED0_ON_L_REG       0x06
#define PCA9685_CHANNEL_STRIDE      4

#define LOCK_SERVO_CHANNEL          0
#define LOCK_SERVO_PWM_LOCKED       200   // one end of travel
#define LOCK_SERVO_PWM_UNLOCKED     300   // other end of travel
#define LOCK_SERVO_PWM_ON           0
#define PCA9685_SERVO_FREQ_HZ       50
// Sweep: step size and delay control servo speed.
// At 50Hz one PWM frame = 20ms. Step every 20ms = one frame per step.
// 100 counts of travel × 20ms = 2 seconds full travel (half of instant).
#define SERVO_SWEEP_STEP_SIZE       7    // counts per step
#define SERVO_SWEEP_STEP_DELAY_MS   5   // ms between steps

static uint16_t s_current_pwm = LOCK_SERVO_PWM_LOCKED;

typedef struct {
    uint16_t target;
} servo_sweep_args_t;

static esp_err_t ensure_i2c_ready(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    // Try to configure the I2C port. If already configured, that's fine — the bus is usable.
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err == ESP_ERR_INVALID_STATE || err == ESP_ERR_INVALID_ARG) {
        // Port already configured (by waveshare_lcd_port or another peripheral), reuse existing bus
        ESP_LOGI(TAG, "I2C port %d already configured, reusing existing bus", I2C_MASTER_NUM);
        return ESP_OK;
    } else if (err != ESP_OK) {
        return err;
    }

    // Try to install the driver. If already installed, that's also fine.
    err = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER,
                             I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err == ESP_ERR_INVALID_STATE || err == ESP_FAIL) {
        // Driver already installed, reuse it
        ESP_LOGI(TAG, "I2C driver already installed on port %d, reusing", I2C_MASTER_NUM);
        return ESP_OK;
    }
    return err;
}

static esp_err_t pca9685_write_reg(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_write_to_device(I2C_MASTER_NUM, PCA9685_I2C_ADDR, data, sizeof(data),
                                      pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

static esp_err_t pca9685_write_pwm(uint8_t channel, uint16_t on_count, uint16_t off_count)
{
    uint8_t reg = PCA9685_LED0_ON_L_REG + (channel * PCA9685_CHANNEL_STRIDE);
    uint8_t payload[5] = {
        reg,
        (uint8_t)(on_count & 0xFF),
        (uint8_t)((on_count >> 8) & 0x0F),
        (uint8_t)(off_count & 0xFF),
        (uint8_t)((off_count >> 8) & 0x0F),
    };
    return i2c_master_write_to_device(I2C_MASTER_NUM, PCA9685_I2C_ADDR, payload, sizeof(payload),
                                      pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

static esp_err_t pca9685_set_frequency(uint16_t freq_hz)
{
    if (freq_hz == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Typical PCA9685 prescale formula with 25MHz internal oscillator.
    float prescale = ((25000000.0f / (4096.0f * (float)freq_hz)) - 1.0f) + 0.5f;
    uint8_t prescale_val = (uint8_t)prescale;

    esp_err_t err = pca9685_write_reg(PCA9685_MODE1_REG, 0x10);  // sleep
    if (err != ESP_OK) return err;
    err = pca9685_write_reg(PCA9685_PRESCALE_REG, prescale_val);
    if (err != ESP_OK) return err;
    err = pca9685_write_reg(PCA9685_MODE1_REG, 0x80);            // reset
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(5));
    return pca9685_write_reg(PCA9685_MODE1_REG, 0xA0);           // auto-increment
}

esp_err_t pca9685_device_init(void)
{
#if !CONFIG_LOCKBOX_INTEGRATION_ENABLE || !CONFIG_LOCKBOX_FEATURE_PCA9685
    return ESP_ERR_NOT_SUPPORTED;
#elif CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    s_initialized = true;
    ESP_LOGI(TAG, "Initialized in mock mode");
    return ESP_OK;
#else
    if (s_initialized) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "I2C already initialized, configuring PCA9685 at 0x%02X", PCA9685_I2C_ADDR);

    esp_err_t err = pca9685_set_frequency(PCA9685_SERVO_FREQ_HZ);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PCA9685 frequency set failed (I2C error? wrong address?): %s", esp_err_to_name(err));
        return err;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "PCA9685 initialized at address 0x%02X", PCA9685_I2C_ADDR);
    return ESP_OK;
#endif
}

static void servo_sweep_task(void *arg)
{
    servo_sweep_args_t *args = (servo_sweep_args_t *)arg;
    uint16_t target = args->target;
    free(args);

    while (s_current_pwm != target) {
        if (s_current_pwm < target) {
            s_current_pwm += SERVO_SWEEP_STEP_SIZE;
            if (s_current_pwm > target) s_current_pwm = target;
        } else {
            s_current_pwm -= SERVO_SWEEP_STEP_SIZE;
            if (s_current_pwm < target) s_current_pwm = target;
        }
        pca9685_write_pwm(LOCK_SERVO_CHANNEL, LOCK_SERVO_PWM_ON, s_current_pwm);
        vTaskDelay(pdMS_TO_TICKS(SERVO_SWEEP_STEP_DELAY_MS));
    }
    ESP_LOGI(TAG, "Servo sweep complete: pwm=%u", s_current_pwm);
    vTaskDelete(NULL);
}

esp_err_t pca9685_device_set_lock_position(lock_servo_position_t position)
{
    if (!s_initialized) {
        esp_err_t init_err = pca9685_device_init();
        if (init_err != ESP_OK) {
            return init_err;
        }
    }

#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    (void)position;
    return ESP_OK;
#else
    uint16_t target = (position == LOCK_SERVO_POSITION_LOCKED)
                        ? LOCK_SERVO_PWM_LOCKED
                        : LOCK_SERVO_PWM_UNLOCKED;

    servo_sweep_args_t *args = malloc(sizeof(servo_sweep_args_t));
    if (!args) return ESP_ERR_NO_MEM;
    args->target = target;

    BaseType_t ok = xTaskCreate(servo_sweep_task, "servo_sweep", 2048,
                                args, tskIDLE_PRIORITY + 1, NULL);
    if (ok != pdPASS) {
        free(args);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
#endif
}
