#include "devices/r503_device.h"
#include "comm/sc16is752_transport.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "R503";

#define R503_ADDR_BYTE              0xFF
#define R503_PACKET_HEADER_LEN      9
#define R503_PACKET_MIN_LEN         12
#define R503_PACKET_BUF_MAX         64

#define R503_PACKET_CMD             0x01
#define R503_PACKET_ACK             0x07

#define R503_CMD_GET_IMAGE          0x01
#define R503_CMD_IMAGE2TZ           0x02
#define R503_CMD_SEARCH             0x04
#define R503_CMD_REG_MODEL          0x05
#define R503_CMD_STORE              0x06
#define R503_CMD_DELETE             0x0C
#define R503_CMD_LED_CONTROL        0x35
#define R503_CMD_AUTO_IDENTIFY      0x32

#define R503_LED_BREATHING          0x01
#define R503_LED_FLASHING           0x02
#define R503_LED_ON                 0x03
#define R503_LED_OFF_MODE           0x04
#define R503_LED_RED                0x01
#define R503_LED_BLUE               0x02
#define R503_LED_PURPLE             0x03
#define R503_LED_GREEN              0x04
#define R503_LED_YELLOW             0x06
#define R503_LED_WHITE              0x07

// AutoIdentify (0x32) timeout: sensor waits internally for finger placement.
// Allow up to 8 seconds for the sensor to wait, capture, and process.
#define R503_AUTO_IDENTIFY_TIMEOUT_MS 8000

#define R503_RESP_OK                0x00
#define R503_RESP_PACKET_ERR        0x01
#define R503_RESP_NO_FINGER         0x02
#define R503_RESP_NO_MATCH          0x08
#define R503_RESP_NOT_FOUND         0x09
#define R503_RESP_ENROLL_MISMATCH   0x0A
#define R503_RESP_BAD_LOCATION      0x0B
#define R503_RESP_DELETE_FAIL       0x10

static bool s_r503_ready = false;

static uint16_t packet_checksum(const uint8_t *payload, size_t payload_len)
{
    uint16_t length = (uint16_t)(payload_len + 2);
    uint16_t sum = R503_PACKET_CMD;
    sum += (uint8_t)((length >> 8) & 0xFF);
    sum += (uint8_t)(length & 0xFF);

    for (size_t i = 0; i < payload_len; i++) {
        sum += payload[i];
    }
    return sum;
}

static size_t build_command_packet(const uint8_t *payload, size_t payload_len, uint8_t *out_pkt, size_t out_max)
{
    const size_t total_len = R503_PACKET_HEADER_LEN + payload_len + 2;
    if (out_pkt == NULL || payload == NULL || out_max < total_len) {
        return 0;
    }

    uint16_t length = (uint16_t)(payload_len + 2);
    out_pkt[0] = 0xEF;
    out_pkt[1] = 0x01;
    out_pkt[2] = R503_ADDR_BYTE;
    out_pkt[3] = R503_ADDR_BYTE;
    out_pkt[4] = R503_ADDR_BYTE;
    out_pkt[5] = R503_ADDR_BYTE;
    out_pkt[6] = R503_PACKET_CMD;
    out_pkt[7] = (uint8_t)((length >> 8) & 0xFF);
    out_pkt[8] = (uint8_t)(length & 0xFF);

    for (size_t i = 0; i < payload_len; i++) {
        out_pkt[R503_PACKET_HEADER_LEN + i] = payload[i];
    }

    uint16_t checksum = packet_checksum(payload, payload_len);
    out_pkt[R503_PACKET_HEADER_LEN + payload_len] = (uint8_t)((checksum >> 8) & 0xFF);
    out_pkt[R503_PACKET_HEADER_LEN + payload_len + 1] = (uint8_t)(checksum & 0xFF);
    return total_len;
}

static r503_status_t map_response_code(uint8_t code)
{
    switch (code) {
        case R503_RESP_OK:
            return R503_STATUS_OK;
        case R503_RESP_NO_FINGER:
            return R503_STATUS_NO_FINGER;
        case R503_RESP_NO_MATCH:
        case R503_RESP_NOT_FOUND:
            return R503_STATUS_NO_MATCH;
        case R503_RESP_BAD_LOCATION:
            return R503_STATUS_BAD_LOCATION;
        case R503_RESP_ENROLL_MISMATCH:
            return R503_STATUS_ENROLL_MISMATCH;
        case R503_RESP_DELETE_FAIL:
            return R503_STATUS_DELETE_FAILED;
        case R503_RESP_PACKET_ERR:
        default:
            return R503_STATUS_SENSOR_ERROR;
    }
}

static esp_err_t send_r503_command(const uint8_t *cmd_payload, size_t cmd_len,
                                   uint8_t *ack_payload, size_t ack_payload_max, size_t *out_ack_payload_len)
{
    uint8_t tx_buf[R503_PACKET_BUF_MAX];
    uint8_t rx_buf[R503_PACKET_BUF_MAX];
    size_t rx_len = 0;
    size_t tx_len = build_command_packet(cmd_payload, cmd_len, tx_buf, sizeof(tx_buf));
    if (tx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = sc16is752_transport_exchange(LOCKBOX_COMM_CHANNEL_FINGERPRINT,
                                                 tx_buf, tx_len, rx_buf, sizeof(rx_buf), &rx_len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "transport_exchange failed: %s", esp_err_to_name(err));
        return (err == ESP_ERR_TIMEOUT) ? ESP_ERR_TIMEOUT : ESP_ERR_INVALID_RESPONSE;
    }
    if (rx_len < R503_PACKET_MIN_LEN) {
        ESP_LOGW(TAG, "No/short response: got %u bytes (need %d) — R503 not replying",
                 (unsigned)rx_len, R503_PACKET_MIN_LEN);
        return ESP_ERR_TIMEOUT;
    }
    if (rx_buf[0] != 0xEF || rx_buf[1] != 0x01 || rx_buf[6] != R503_PACKET_ACK) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint16_t pkt_len = (uint16_t)(((uint16_t)rx_buf[7] << 8) | rx_buf[8]);
    if (pkt_len < 3 || (size_t)(pkt_len + R503_PACKET_HEADER_LEN) > rx_len) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    size_t payload_len = pkt_len - 2;
    if (ack_payload && ack_payload_max > 0) {
        if (payload_len > ack_payload_max) {
            payload_len = ack_payload_max;
        }
        for (size_t i = 0; i < payload_len; i++) {
            ack_payload[i] = rx_buf[R503_PACKET_HEADER_LEN + i];
        }
        if (out_ack_payload_len) {
            *out_ack_payload_len = payload_len;
        }
    } else if (out_ack_payload_len) {
        *out_ack_payload_len = payload_len;
    }

    return ESP_OK;
}

static esp_err_t send_r503_command_timed(const uint8_t *cmd_payload, size_t cmd_len,
                                         uint8_t *ack_payload, size_t ack_payload_max, size_t *out_ack_payload_len,
                                         int first_byte_timeout_ms)
{
    uint8_t tx_buf[R503_PACKET_BUF_MAX];
    uint8_t rx_buf[R503_PACKET_BUF_MAX];
    size_t rx_len = 0;
    size_t tx_len = build_command_packet(cmd_payload, cmd_len, tx_buf, sizeof(tx_buf));
    if (tx_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = sc16is752_transport_exchange_timed(LOCKBOX_COMM_CHANNEL_FINGERPRINT,
                                                       tx_buf, tx_len, rx_buf, sizeof(rx_buf), &rx_len,
                                                       first_byte_timeout_ms);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "transport_exchange_timed failed: %s", esp_err_to_name(err));
        return (err == ESP_ERR_TIMEOUT) ? ESP_ERR_TIMEOUT : ESP_ERR_INVALID_RESPONSE;
    }
    if (rx_len < R503_PACKET_MIN_LEN) {
        ESP_LOGW(TAG, "No/short response: got %u bytes (need %d)", (unsigned)rx_len, R503_PACKET_MIN_LEN);
        return ESP_ERR_TIMEOUT;
    }
    if (rx_buf[0] != 0xEF || rx_buf[1] != 0x01 || rx_buf[6] != R503_PACKET_ACK) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint16_t pkt_len = (uint16_t)(((uint16_t)rx_buf[7] << 8) | rx_buf[8]);
    if (pkt_len < 3 || (size_t)(pkt_len + R503_PACKET_HEADER_LEN) > rx_len) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    size_t payload_len = pkt_len - 2;
    if (ack_payload && ack_payload_max > 0) {
        if (payload_len > ack_payload_max) {
            payload_len = ack_payload_max;
        }
        for (size_t i = 0; i < payload_len; i++) {
            ack_payload[i] = rx_buf[R503_PACKET_HEADER_LEN + i];
        }
        if (out_ack_payload_len) {
            *out_ack_payload_len = payload_len;
        }
    } else if (out_ack_payload_len) {
        *out_ack_payload_len = payload_len;
    }

    return ESP_OK;
}

static void r503_led(uint8_t control, uint8_t speed, uint8_t color, uint8_t times)
{
    const uint8_t cmd[] = {R503_CMD_LED_CONTROL, control, speed, color, times};
    uint8_t ack[4];
    size_t ack_len = 0;
    // Best-effort: ignore errors — LED is cosmetic, not critical
    send_r503_command(cmd, sizeof(cmd), ack, sizeof(ack), &ack_len);
}

static r503_status_t capture_image_with_retry(int timeout_ms)
{
    const int64_t deadline = esp_timer_get_time() + ((int64_t)timeout_ms * 1000);
    const uint8_t cmd[] = {R503_CMD_GET_IMAGE};

    while (esp_timer_get_time() < deadline) {
        uint8_t ack[8];
        size_t ack_len = 0;
        esp_err_t err = send_r503_command(cmd, sizeof(cmd), ack, sizeof(ack), &ack_len);
        if (err != ESP_OK) {
            return (err == ESP_ERR_TIMEOUT) ? R503_STATUS_TIMEOUT : R503_STATUS_COMM_ERROR;
        }
        if (ack_len == 0) {
            return R503_STATUS_SENSOR_ERROR;
        }

        r503_status_t status = map_response_code(ack[0]);
        if (status == R503_STATUS_NO_FINGER) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }
        return status;
    }
    return R503_STATUS_TIMEOUT;
}

static r503_status_t image_to_template(uint8_t slot)
{
    const uint8_t cmd[] = {R503_CMD_IMAGE2TZ, slot};
    uint8_t ack[8];
    size_t ack_len = 0;
    esp_err_t err = send_r503_command(cmd, sizeof(cmd), ack, sizeof(ack), &ack_len);
    if (err != ESP_OK || ack_len == 0) {
        return (err == ESP_ERR_TIMEOUT) ? R503_STATUS_TIMEOUT : R503_STATUS_COMM_ERROR;
    }
    return map_response_code(ack[0]);
}

const char *r503_status_to_string(r503_status_t status)
{
    switch (status) {
        case R503_STATUS_OK: return "ok";
        case R503_STATUS_NO_FINGER: return "no_finger";
        case R503_STATUS_NO_MATCH: return "no_match";
        case R503_STATUS_BAD_LOCATION: return "bad_location";
        case R503_STATUS_ENROLL_MISMATCH: return "enroll_mismatch";
        case R503_STATUS_DELETE_FAILED: return "delete_failed";
        case R503_STATUS_SENSOR_ERROR: return "sensor_error";
        case R503_STATUS_TIMEOUT: return "timeout";
        case R503_STATUS_COMM_ERROR: return "comm_error";
        default: return "unknown";
    }
}

esp_err_t r503_device_init(void)
{
#if !CONFIG_LOCKBOX_INTEGRATION_ENABLE || !CONFIG_LOCKBOX_FEATURE_R503
    s_r503_ready = false;
    return ESP_ERR_NOT_SUPPORTED;
#else
    esp_err_t err = sc16is752_transport_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Transport init failed: %s", esp_err_to_name(err));
        s_r503_ready = false;
        return err;
    }
    err = sc16is752_transport_configure_channel(LOCKBOX_COMM_CHANNEL_FINGERPRINT, 57600);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "CH_A configure failed: %s", esp_err_to_name(err));
        s_r503_ready = false;
        return err;
    }
    s_r503_ready = true;
    ESP_LOGI(TAG, "R503 device ready on SC16IS752 CH_A at 57600 baud");
    return ESP_OK;
#endif
}

esp_err_t r503_device_match(r503_match_result_t *result)
{
    if (result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_r503_ready) {
        esp_err_t init_err = r503_device_init();
        if (init_err != ESP_OK) {
            result->status = R503_STATUS_COMM_ERROR;
            return init_err;
        }
    }

    result->status = R503_STATUS_NO_MATCH;
    result->matched = false;
    result->matched_userid = -1;
    result->matched_template_id = -1;
    result->confidence = 0;

    // Breathing WHITE: signals "place finger now"
    r503_led(R503_LED_BREATHING, 128, R503_LED_WHITE, 0);

    // AutoIdentify: sensor waits for finger, captures image, generates template, searches library.
    // Params: BufferID=1, score_level=5, start_page=0, page_count=1000
    const uint8_t cmd[] = {R503_CMD_AUTO_IDENTIFY, 0x01, 0x05, 0x00, 0x00, 0x03, 0xE8};
    uint8_t ack[8];
    size_t ack_len = 0;
    esp_err_t err = send_r503_command_timed(cmd, sizeof(cmd), ack, sizeof(ack), &ack_len,
                                            R503_AUTO_IDENTIFY_TIMEOUT_MS);
    if (err != ESP_OK || ack_len == 0) {
        r503_led(R503_LED_FLASHING, 150, R503_LED_RED, 3);
        result->status = (err == ESP_ERR_TIMEOUT) ? R503_STATUS_TIMEOUT : R503_STATUS_COMM_ERROR;
        ESP_LOGW(TAG, "AutoIdentify transport error: %s", esp_err_to_name(err));
        return ESP_OK;
    }

    uint8_t resp_code = ack[0];
    ESP_LOGI(TAG, "AutoIdentify response: 0x%02X (ack_len=%u)", resp_code, (unsigned)ack_len);

    if (resp_code == R503_RESP_OK && ack_len >= 5) {
        result->matched = true;
        result->matched_template_id = (int)(((uint16_t)ack[1] << 8) | ack[2]);
        result->confidence = (uint16_t)(((uint16_t)ack[3] << 8) | ack[4]);
        result->status = R503_STATUS_OK;
        r503_led(R503_LED_FLASHING, 150, R503_LED_GREEN, 3);
        ESP_LOGI(TAG, "AutoIdentify match: template_id=%d confidence=%u",
                 result->matched_template_id, result->confidence);
    } else if (resp_code == R503_RESP_NO_FINGER) {
        result->status = R503_STATUS_NO_FINGER;
        // Blue 3x then Yellow 3x for "no finger placed"
        r503_led(R503_LED_FLASHING, 150, R503_LED_BLUE, 3);
        vTaskDelay(pdMS_TO_TICKS(1000)); // wait for blue sequence to finish
        r503_led(R503_LED_FLASHING, 150, R503_LED_YELLOW, 3);
    } else if (resp_code == R503_RESP_NO_MATCH || resp_code == R503_RESP_NOT_FOUND) {
        result->status = R503_STATUS_NO_MATCH;
        // Blue 3x then Red 3x for "finger found but no match in library"
        r503_led(R503_LED_FLASHING, 150, R503_LED_BLUE, 3);
        vTaskDelay(pdMS_TO_TICKS(1000)); // wait for blue sequence to finish
        r503_led(R503_LED_FLASHING, 150, R503_LED_RED, 3);
    } else {
        result->status = R503_STATUS_SENSOR_ERROR;
        r503_led(R503_LED_FLASHING, 150, R503_LED_RED, 3);
        ESP_LOGW(TAG, "AutoIdentify error code: 0x%02X", resp_code);
    }
    return ESP_OK;
}

esp_err_t r503_device_enroll(int userid, int *out_template_id, r503_status_t *out_status)
{
    if (userid < 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_r503_ready) {
        esp_err_t init_err = r503_device_init();
        if (init_err != ESP_OK) {
            if (out_status) *out_status = R503_STATUS_COMM_ERROR;
            return init_err;
        }
    }

    if (out_template_id) {
        *out_template_id = -1;
    }
    if (out_status) {
        *out_status = R503_STATUS_SENSOR_ERROR;
    }

    r503_status_t status = capture_image_with_retry(10000);
    if (status != R503_STATUS_OK) {
        if (out_status) *out_status = status;
        return ESP_OK;
    }

    status = image_to_template(1);
    if (status != R503_STATUS_OK) {
        if (out_status) *out_status = status;
        return ESP_OK;
    }

    int64_t remove_deadline = esp_timer_get_time() + (8000LL * 1000);
    while (esp_timer_get_time() < remove_deadline) {
        const uint8_t get_img_cmd[] = {R503_CMD_GET_IMAGE};
        uint8_t ack[8];
        size_t ack_len = 0;
        esp_err_t err = send_r503_command(get_img_cmd, sizeof(get_img_cmd), ack, sizeof(ack), &ack_len);
        if (err != ESP_OK || ack_len == 0) {
            if (out_status) *out_status = (err == ESP_ERR_TIMEOUT) ? R503_STATUS_TIMEOUT : R503_STATUS_COMM_ERROR;
            return ESP_OK;
        }
        if (map_response_code(ack[0]) == R503_STATUS_NO_FINGER) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    status = capture_image_with_retry(10000);
    if (status != R503_STATUS_OK) {
        if (out_status) *out_status = status;
        return ESP_OK;
    }

    status = image_to_template(2);
    if (status != R503_STATUS_OK) {
        if (out_status) *out_status = status;
        return ESP_OK;
    }

    const uint8_t reg_model_cmd[] = {R503_CMD_REG_MODEL};
    uint8_t ack[8];
    size_t ack_len = 0;
    esp_err_t err = send_r503_command(reg_model_cmd, sizeof(reg_model_cmd), ack, sizeof(ack), &ack_len);
    if (err != ESP_OK || ack_len == 0) {
        if (out_status) *out_status = (err == ESP_ERR_TIMEOUT) ? R503_STATUS_TIMEOUT : R503_STATUS_COMM_ERROR;
        return ESP_OK;
    }
    status = map_response_code(ack[0]);
    if (status != R503_STATUS_OK) {
        if (out_status) *out_status = status;
        return ESP_OK;
    }

    uint16_t template_id = (uint16_t)userid;
    const uint8_t store_cmd[] = {R503_CMD_STORE, 0x01,
                                 (uint8_t)((template_id >> 8) & 0xFF),
                                 (uint8_t)(template_id & 0xFF)};
    ack_len = 0;
    err = send_r503_command(store_cmd, sizeof(store_cmd), ack, sizeof(ack), &ack_len);
    if (err != ESP_OK || ack_len == 0) {
        if (out_status) *out_status = (err == ESP_ERR_TIMEOUT) ? R503_STATUS_TIMEOUT : R503_STATUS_COMM_ERROR;
        return ESP_OK;
    }
    status = map_response_code(ack[0]);
    if (out_status) *out_status = status;
    if (status == R503_STATUS_OK && out_template_id) {
        *out_template_id = (int)template_id;
    }
    return ESP_OK;
}

esp_err_t r503_device_delete_template(int template_id, r503_status_t *out_status)
{
    if (template_id < 0 || template_id > 0xFFFF) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_r503_ready) {
        esp_err_t init_err = r503_device_init();
        if (init_err != ESP_OK) {
            if (out_status) *out_status = R503_STATUS_COMM_ERROR;
            return init_err;
        }
    }

    const uint8_t del_cmd[] = {R503_CMD_DELETE,
                               (uint8_t)((template_id >> 8) & 0xFF),
                               (uint8_t)(template_id & 0xFF),
                               0x00, 0x01};
    uint8_t ack[8];
    size_t ack_len = 0;
    esp_err_t err = send_r503_command(del_cmd, sizeof(del_cmd), ack, sizeof(ack), &ack_len);
    if (err != ESP_OK || ack_len == 0) {
        if (out_status) *out_status = (err == ESP_ERR_TIMEOUT) ? R503_STATUS_TIMEOUT : R503_STATUS_COMM_ERROR;
        return ESP_OK;
    }

    if (out_status) {
        *out_status = map_response_code(ack[0]);
    }
    return ESP_OK;
}
