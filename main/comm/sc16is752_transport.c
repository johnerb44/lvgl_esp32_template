#include "comm/sc16is752_transport.h"
#include "sd/sd_card.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "SC16IS752_TRANSPORT";
static bool s_transport_ready = false;
static bool s_channels_configured = false;

/* Mock R503 touch simulation for testing sleep wake detection */
static volatile bool s_r503_mock_touched = false;

#define SC16IS752_ADDR_DEFAULT      0x4D
#define SC16IS752_CH_A              0x00
#define SC16IS752_CH_B              0x01

#define SC16IS752_RHR_REG           (0x00 << 3)
#define SC16IS752_THR_REG           (0x00 << 3)
#define SC16IS752_IER_REG           (0x01 << 3)
#define SC16IS752_FCR_REG           (0x02 << 3)
#define SC16IS752_LCR_REG           (0x03 << 3)
#define SC16IS752_LSR_REG           (0x05 << 3)
#define SC16IS752_IODIR_REG         (0x0A << 3)
#define SC16IS752_IOSTATE_REG       (0x0B << 3)
#define SC16IS752_IOCONTROL_REG     (0x0E << 3)
#define SC16IS752_SPR_REG           (0x07 << 3)
#define SC16IS752_DLL_REG           (0x00 << 3)
#define SC16IS752_DLH_REG           (0x01 << 3)
#define SC16IS752_EFR_REG           (0x02 << 3)

#define SC16IS752_LCR_DIVISOR_EN    0x80
#define SC16IS752_LCR_ENHANCED      0xBF
#define SC16IS752_LCR_8N1           0x03
#define SC16IS752_EFR_ENABLE        0x10
#define SC16IS752_FCR_ENABLE        0x01
#define SC16IS752_FCR_RX_RESET      0x02
#define SC16IS752_FCR_TX_RESET      0x04
#define SC16IS752_LSR_DR            0x01
#define SC16IS752_LSR_THRE          0x20
#define SC16IS752_LSR_TEMT          0x40

static uint8_t channel_to_sc16_channel(lockbox_comm_channel_t channel)
{
    if (channel == LOCKBOX_COMM_CHANNEL_FACE) {
        return SC16IS752_CH_B;
    }
    return SC16IS752_CH_A;
}

static esp_err_t ensure_i2c_ready(void)
{
    // Try to install the driver first. In ESP-IDF v5.1 the legacy I2C driver
    // returns ESP_FAIL (logged as "i2c driver install error") when the driver
    // is already installed. ESP_ERR_INVALID_STATE is returned by some versions.
    // Either way the bus is usable — skip param_config to avoid disrupting the
    // LCD/touch subsystem that owns the bus.
    esp_err_t err = i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER,
                                       I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err == ESP_OK) {
        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = I2C_MASTER_FREQ_HZ,
        };
        return i2c_param_config(I2C_MASTER_NUM, &conf);
    }
    if (err == ESP_FAIL || err == ESP_ERR_INVALID_STATE) {
        ESP_LOGI(TAG, "I2C port %d already installed, reusing existing bus", I2C_MASTER_NUM);
        return ESP_OK;
    }
    return err;
}

static esp_err_t sc16_write_reg(uint8_t sc16_channel, uint8_t reg_addr, uint8_t data)
{
    uint8_t channel_reg_addr = reg_addr | (sc16_channel << 1);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SC16IS752_ADDR_DEFAULT << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, channel_reg_addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t sc16_read_reg(uint8_t sc16_channel, uint8_t reg_addr, uint8_t *out_data)
{
    if (out_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t channel_reg_addr = reg_addr | (sc16_channel << 1);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SC16IS752_ADDR_DEFAULT << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, channel_reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SC16IS752_ADDR_DEFAULT << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, out_data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t wait_for_tx_ready(uint8_t sc16_channel, int timeout_ms)
{
    int64_t deadline = esp_timer_get_time() + ((int64_t)timeout_ms * 1000);
    while (esp_timer_get_time() < deadline) {
        uint8_t lsr = 0;
        esp_err_t err = sc16_read_reg(sc16_channel, SC16IS752_LSR_REG, &lsr);
        if (err != ESP_OK) {
            return err;
        }
        if (lsr & SC16IS752_LSR_THRE) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t wait_for_tx_complete(uint8_t sc16_channel, int timeout_ms)
{
    int64_t deadline = esp_timer_get_time() + ((int64_t)timeout_ms * 1000);
    while (esp_timer_get_time() < deadline) {
        uint8_t lsr = 0;
        esp_err_t err = sc16_read_reg(sc16_channel, SC16IS752_LSR_REG, &lsr);
        if (err != ESP_OK) {
            return err;
        }
        if ((lsr & (SC16IS752_LSR_THRE | SC16IS752_LSR_TEMT)) == (SC16IS752_LSR_THRE | SC16IS752_LSR_TEMT)) {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return ESP_ERR_TIMEOUT;
}

static esp_err_t configure_sc16_uart(uint8_t sc16_channel, uint32_t baud_rate)
{
    uint8_t dll = 0;
    uint8_t dlh = 0;
    if (baud_rate == 57600) {
        dll = 0x02;
        dlh = 0x00;
    } else if (baud_rate == 115200) {
        dll = 0x01;
        dlh = 0x00;
    } else {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = sc16_write_reg(sc16_channel, SC16IS752_LCR_REG, SC16IS752_LCR_DIVISOR_EN);
    if (err != ESP_OK) return err;
    err = sc16_write_reg(sc16_channel, SC16IS752_DLL_REG, dll);
    if (err != ESP_OK) return err;
    err = sc16_write_reg(sc16_channel, SC16IS752_DLH_REG, dlh);
    if (err != ESP_OK) return err;
    err = sc16_write_reg(sc16_channel, SC16IS752_LCR_REG, SC16IS752_LCR_ENHANCED);
    if (err != ESP_OK) return err;
    err = sc16_write_reg(sc16_channel, SC16IS752_EFR_REG, SC16IS752_EFR_ENABLE);
    if (err != ESP_OK) return err;
    err = sc16_write_reg(sc16_channel, SC16IS752_LCR_REG, SC16IS752_LCR_8N1);
    if (err != ESP_OK) return err;
    err = sc16_write_reg(sc16_channel, SC16IS752_FCR_REG,
                         SC16IS752_FCR_ENABLE | SC16IS752_FCR_RX_RESET | SC16IS752_FCR_TX_RESET);
    if (err != ESP_OK) return err;
    return sc16_write_reg(sc16_channel, SC16IS752_IER_REG, 0x00);
}

esp_err_t sc16is752_transport_init(void)
{
#if !CONFIG_LOCKBOX_INTEGRATION_ENABLE
    ESP_LOGW(TAG, "Secure Lockbox integration disabled");
    s_transport_ready = false;
    return ESP_ERR_NOT_SUPPORTED;
#elif !CONFIG_LOCKBOX_FEATURE_SC16IS752
    ESP_LOGW(TAG, "SC16IS752 feature disabled in Kconfig");
    s_transport_ready = false;
    return ESP_ERR_NOT_SUPPORTED;
#elif CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    ESP_LOGI(TAG, "SC16IS752 transport initialized in mock mode");
    s_transport_ready = true;
    s_channels_configured = true;
    return ESP_OK;
#else
    esp_err_t err = ensure_i2c_ready();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2C for SC16IS752: %s", esp_err_to_name(err));
        s_transport_ready = false;
        return err;
    }

    err = configure_sc16_uart(SC16IS752_CH_A, 57600);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure SC16IS752 channel A: %s", esp_err_to_name(err));
        s_transport_ready = false;
        return err;
    }

    err = configure_sc16_uart(SC16IS752_CH_B, 115200);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure SC16IS752 channel B: %s", esp_err_to_name(err));
        s_transport_ready = false;
        return err;
    }

    err = sc16_write_reg(SC16IS752_CH_A, SC16IS752_IOCONTROL_REG, 0x00);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure SC16IS752 GPIO mode: %s", esp_err_to_name(err));
        s_transport_ready = false;
        return err;
    }

    s_channels_configured = true;
    s_transport_ready = true;
    ESP_LOGI(TAG, "SC16IS752 transport initialized (A=57600, B=115200)");
    return ESP_OK;
#endif
}

bool sc16is752_transport_is_ready(void)
{
    return s_transport_ready;
}

esp_err_t sc16is752_transport_probe(void)
{
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    ESP_LOGI(TAG, "Probe: mock mode, reporting ready");
    return ESP_OK;
#else
    const uint8_t test_val = 0x55;
    esp_err_t err = sc16_write_reg(SC16IS752_CH_A, SC16IS752_SPR_REG, test_val);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Probe: SPR write failed (%s) — SC16IS752 not responding on I2C",
                 esp_err_to_name(err));
        return err;
    }
    uint8_t readback = 0;
    err = sc16_read_reg(SC16IS752_CH_A, SC16IS752_SPR_REG, &readback);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Probe: SPR read failed (%s) — I2C reads not working", esp_err_to_name(err));
        return err;
    }
    if (readback != test_val) {
        ESP_LOGE(TAG, "Probe: SPR readback mismatch (wrote 0x%02X, got 0x%02X) — SC16IS752 not properly responding",
                 test_val, readback);
        return ESP_ERR_INVALID_RESPONSE;
    }
    ESP_LOGI(TAG, "Probe: SC16IS752 verified at I2C addr 0x%02X — SPR readback OK (0x%02X)",
             SC16IS752_ADDR_DEFAULT, readback);
    return ESP_OK;
#endif
}

esp_err_t sc16is752_transport_configure_channel(lockbox_comm_channel_t channel, uint32_t baud_rate)
{
    if (!s_transport_ready) {
        return ESP_ERR_INVALID_STATE;
    }
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    (void)channel;
    (void)baud_rate;
    return ESP_OK;
#else
    return configure_sc16_uart(channel_to_sc16_channel(channel), baud_rate);
#endif
}

esp_err_t sc16is752_transport_write(lockbox_comm_channel_t channel, const uint8_t *data, size_t len)
{
    if (!s_transport_ready || !s_channels_configured) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((data == NULL && len > 0) || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    (void)channel;
    (void)data;
    (void)len;
    return ESP_OK;
#else
    uint8_t sc16_channel = channel_to_sc16_channel(channel);

    for (size_t i = 0; i < len; i++) {
        esp_err_t err = wait_for_tx_ready(sc16_channel, 1000);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "TX: wait_for_tx_ready timed out at byte %u", (unsigned)i);
            return err;
        }
        err = sc16_write_reg(sc16_channel, SC16IS752_THR_REG, data[i]);
        if (err != ESP_OK) {
            return err;
        }
    }
    esp_err_t temt_err = wait_for_tx_complete(sc16_channel, 1000);
    if (temt_err != ESP_OK) {
        ESP_LOGE(TAG, "TX: TEMT timeout — bytes may not have been sent");
    }
    return temt_err;
#endif
}

static esp_err_t sc16is752_transport_read_timed(lockbox_comm_channel_t channel,
                                                uint8_t *data, size_t max_len, size_t *out_len,
                                                int first_byte_timeout_ms)
{
    uint8_t sc16_channel = channel_to_sc16_channel(channel);
    size_t count = 0;
    int64_t first_deadline = esp_timer_get_time() + ((int64_t)first_byte_timeout_ms * 1000);
    while (count < max_len && esp_timer_get_time() < first_deadline) {
        uint8_t lsr = 0;
        esp_err_t err = sc16_read_reg(sc16_channel, SC16IS752_LSR_REG, &lsr);
        if (err != ESP_OK) {
            return err;
        }
        if (lsr & SC16IS752_LSR_DR) {
            err = sc16_read_reg(sc16_channel, SC16IS752_RHR_REG, &data[count]);
            if (err != ESP_OK) {
                return err;
            }
            count++;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    if (count == 0) {
        return ESP_ERR_NOT_FOUND;
    }

    // Read remaining bytes in burst — all should arrive within a few ms of the first.
    int64_t more_deadline = esp_timer_get_time() + (50 * 1000);
    int no_data_streak = 0;
    while (count < max_len && esp_timer_get_time() < more_deadline) {
        uint8_t lsr = 0;
        esp_err_t err = sc16_read_reg(sc16_channel, SC16IS752_LSR_REG, &lsr);
        if (err != ESP_OK) {
            return err;
        }
        if (!(lsr & SC16IS752_LSR_DR)) {
            if (++no_data_streak >= 5) {
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }
        no_data_streak = 0;
        err = sc16_read_reg(sc16_channel, SC16IS752_RHR_REG, &data[count]);
        if (err != ESP_OK) {
            return err;
        }
        count++;
    }

    if (out_len) {
        *out_len = count;
    }
    return ESP_OK;
}

esp_err_t sc16is752_transport_read(lockbox_comm_channel_t channel, uint8_t *data, size_t max_len, size_t *out_len)
{
    if (out_len) {
        *out_len = 0;
    }
    if (!s_transport_ready || !s_channels_configured) {
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    (void)channel;
    return ESP_ERR_NOT_FOUND;
#else
    // R503 can take 100-250ms to process a command before responding.
    // 2000ms covers worst-case latency including I2C bus contention.
    return sc16is752_transport_read_timed(channel, data, max_len, out_len, 2000);
#endif
}

esp_err_t sc16is752_transport_exchange(lockbox_comm_channel_t channel,
                                       const uint8_t *tx_data, size_t tx_len,
                                       uint8_t *rx_data, size_t rx_max_len, size_t *out_rx_len)
{
    if (out_rx_len) {
        *out_rx_len = 0;
    }
    if (!s_transport_ready) {
        return ESP_ERR_INVALID_STATE;
    }
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    (void)channel;
    (void)tx_data;
    (void)tx_len;
    (void)rx_data;
    (void)rx_max_len;
    return ESP_OK;
#else
    if (tx_data == NULL || tx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Non-blocking drain: flush any stale bytes already in the RX FIFO.
    // Do NOT call transport_read() here — it blocks for 100ms waiting for bytes
    // that may not exist, eating into the response window for the command we're
    // about to send. Instead, check DR directly and drain only what is there now.
    {
        uint8_t sc16_ch = channel_to_sc16_channel(channel);
        uint8_t lsr = 0;
        uint8_t discard = 0;
        while (sc16_read_reg(sc16_ch, SC16IS752_LSR_REG, &lsr) == ESP_OK
               && (lsr & SC16IS752_LSR_DR)) {
            sc16_read_reg(sc16_ch, SC16IS752_RHR_REG, &discard);
        }
    }

    esp_err_t err = sc16is752_transport_write(channel, tx_data, tx_len);
    if (err != ESP_OK) {
        return err;
    }

    if (rx_data && rx_max_len > 0) {
        err = sc16is752_transport_read(channel, rx_data, rx_max_len, out_rx_len);
        if (err == ESP_ERR_NOT_FOUND) {
            return ESP_OK;
        }
        return err;
    }
    return ESP_OK;
#endif
}

esp_err_t sc16is752_transport_exchange_timed(lockbox_comm_channel_t channel,
                                             const uint8_t *tx_data, size_t tx_len,
                                             uint8_t *rx_data, size_t rx_max_len, size_t *out_rx_len,
                                             int first_byte_timeout_ms)
{
    if (out_rx_len) {
        *out_rx_len = 0;
    }
    if (!s_transport_ready) {
        return ESP_ERR_INVALID_STATE;
    }
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    (void)channel;
    (void)tx_data;
    (void)tx_len;
    (void)rx_data;
    (void)rx_max_len;
    (void)first_byte_timeout_ms;
    return ESP_OK;
#else
    if (tx_data == NULL || tx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Non-blocking drain of stale RX bytes before sending the command.
    {
        uint8_t sc16_ch = channel_to_sc16_channel(channel);
        uint8_t lsr = 0;
        uint8_t discard = 0;
        while (sc16_read_reg(sc16_ch, SC16IS752_LSR_REG, &lsr) == ESP_OK
               && (lsr & SC16IS752_LSR_DR)) {
            sc16_read_reg(sc16_ch, SC16IS752_RHR_REG, &discard);
        }
    }

    esp_err_t err = sc16is752_transport_write(channel, tx_data, tx_len);
    if (err != ESP_OK) {
        return err;
    }

    if (rx_data && rx_max_len > 0) {
        err = sc16is752_transport_read_timed(channel, rx_data, rx_max_len, out_rx_len, first_byte_timeout_ms);
        if (err == ESP_ERR_NOT_FOUND) {
            return ESP_OK;
        }
        return err;
    }
    return ESP_OK;
#endif
}

esp_err_t sc16is752_transport_gpio_init(uint8_t direction_mask, uint8_t initial_state)
{
    if (!s_transport_ready) {
        return ESP_ERR_INVALID_STATE;
    }
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    (void)direction_mask;
    (void)initial_state;
    return ESP_OK;
#else
    esp_err_t err = sc16_write_reg(SC16IS752_CH_A, SC16IS752_IODIR_REG, direction_mask);
    if (err != ESP_OK) {
        return err;
    }
    return sc16_write_reg(SC16IS752_CH_A, SC16IS752_IOSTATE_REG, initial_state);
#endif
}

esp_err_t sc16is752_transport_gpio_read(uint8_t *out_state)
{
    if (out_state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_transport_ready) {
        return ESP_ERR_INVALID_STATE;
    }
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    /* Default: GP pins = 0x00 (all LOW). GP1=bit1=0 means R503 touched.
     * Override with sc16is752_transport_r503_set_mock_touch() for testing. */
    *out_state = s_r503_mock_touched ? 0x02 : 0x00;
    return ESP_OK;
#else
    return sc16_read_reg(SC16IS752_CH_A, SC16IS752_IOSTATE_REG, out_state);
#endif
}

esp_err_t sc16is752_transport_gpio_write(uint8_t state)
{
    if (!s_transport_ready) {
        return ESP_ERR_INVALID_STATE;
    }
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    (void)state;
    return ESP_OK;
#else
    return sc16_write_reg(SC16IS752_CH_A, SC16IS752_IOSTATE_REG, state);
#endif
}

/* ---- Mock R503 touch simulation ---- */

void sc16is752_transport_r503_set_mock_touch(bool touched)
{
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    s_r503_mock_touched = touched;
#else
    (void)touched;
#endif
}

bool sc16is752_transport_r503_get_mock_touch(void)
{
#if CONFIG_LOCKBOX_INTEGRATION_USE_MOCK_DEVICES
    return s_r503_mock_touched;
#else
    return false;
#endif
}