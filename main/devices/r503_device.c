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
#define R503_CMD_AUTO_ENROLL        0x31

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
#define R503_AUTO_IDENTIFY_TIMEOUT_MS   8000

// AutoEnroll (0x31) timeout: 6 image collections with finger lift between each.
// ~5-8s per scan cycle × 6 = up to 50s, plus processing. Allow 60s.
#define R503_AUTO_ENROLL_TIMEOUT_MS     60000

#define R503_RESP_OK                0x00
#define R503_RESP_PACKET_ERR        0x01  // general failure / malformed packet
#define R503_RESP_NO_FINGER         0x02
#define R503_RESP_NO_MATCH          0x08
#define R503_RESP_NOT_FOUND         0x09
#define R503_RESP_MERGE_FAIL        0x0A  // AutoEnroll: failed to merge templates
#define R503_RESP_BAD_LOCATION      0x0B
#define R503_RESP_DELETE_FAIL       0x10
#define R503_RESP_GENERATE_FAIL     0x07  // AutoEnroll: failed to generate feature
#define R503_RESP_LIBRARY_FULL      0x1F  // AutoEnroll: fingerprint library is full
#define R503_RESP_TEMPLATE_EMPTY    0x22  // AutoEnroll: template slot is empty
#define R503_RESP_ENROLL_TIMEOUT    0x26  // AutoEnroll: sensor internal timeout
#define R503_RESP_DUPLICATE         0x27  // AutoEnroll: fingerprint already exists

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
        case R503_RESP_OK:              return R503_STATUS_OK;
        case R503_RESP_NO_FINGER:       return R503_STATUS_NO_FINGER;
        case R503_RESP_NO_MATCH:
        case R503_RESP_NOT_FOUND:       return R503_STATUS_NO_MATCH;
        case R503_RESP_BAD_LOCATION:    return R503_STATUS_BAD_LOCATION;
        case R503_RESP_MERGE_FAIL:      return R503_STATUS_ENROLL_MISMATCH;
        case R503_RESP_DELETE_FAIL:     return R503_STATUS_DELETE_FAILED;
        case R503_RESP_DUPLICATE:       return R503_STATUS_DUPLICATE;
        case R503_RESP_LIBRARY_FULL:    return R503_STATUS_LIBRARY_FULL;
        case R503_RESP_GENERATE_FAIL:
        case R503_RESP_TEMPLATE_EMPTY:
        case R503_RESP_PACKET_ERR:
        default:                        return R503_STATUS_SENSOR_ERROR;
        case R503_RESP_ENROLL_TIMEOUT:  return R503_STATUS_TIMEOUT;
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

// Read one ACK packet from the sensor without sending a command.
// Used to read additional responses from AutoIdentify (which sends multiple packets).
static esp_err_t read_r503_ack(uint8_t *ack_payload, size_t ack_payload_max, size_t *out_ack_payload_len)
{
    if (out_ack_payload_len) {
        *out_ack_payload_len = 0;
    }
    uint8_t rx_buf[R503_PACKET_BUF_MAX];
    size_t rx_len = 0;
    esp_err_t err = sc16is752_transport_read(LOCKBOX_COMM_CHANNEL_FINGERPRINT,
                                             rx_buf, sizeof(rx_buf), &rx_len);
    if (err != ESP_OK || rx_len < R503_PACKET_MIN_LEN) {
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
        case R503_STATUS_DUPLICATE: return "duplicate";
        case R503_STATUS_LIBRARY_FULL: return "library_full";
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

    // Breathing WHITE: signals "place your finger now"
    r503_led(R503_LED_BREATHING, 128, R503_LED_WHITE, 0);

    // AutoIdentify (0x32) executes 3 steps internally and sends one ACK packet per step:
    //   Packet 1 — Collect Image:    confirm=0x00 if image captured, 0x02 if no finger
    //   Packet 2 — Generate Feature: confirm=0x00 if template generated, else error code
    //   Packet 3 — Search Library:   confirm=0x00 + non-zero ModelID/score if matched,
    //                                 or 0x08/0x09 if no match found
    //
    // Command params (R503-M22 manual, byte-by-byte):
    //   0x03 = security level (1–5; 3 = medium)
    //   0x00 = start search location (page 0)
    //   0xC8 = end search location  (page 200 = full R503-M22 library)
    //   0x01 = response mode (1 = send one ACK packet per key step → 3 packets total)
    //   0x01 = attempts (number of times to run all 3 steps; increase for difficult fingers)
    const uint8_t cmd[] = {R503_CMD_AUTO_IDENTIFY, 0x03, 0x00, 0xC8, 0x01, 0x01};
    uint8_t ack[16];
    size_t ack_len = 0;
    esp_err_t err = send_r503_command_timed(cmd, sizeof(cmd), ack, sizeof(ack), &ack_len,
                                            R503_AUTO_IDENTIFY_TIMEOUT_MS);

    // Loop reads one ACK packet per step (3 steps total).
    // Steps 1 and 2 return confirm=0x00 with all-zero data (no match info yet).
    // Step 3 returns the final result: confirm=0x00 with ModelID+score if matched,
    // or a non-zero confirm code if no match / no finger / error.
    for (int read_count = 0; read_count < 5; read_count++) {
        if (err != ESP_OK || ack_len < 1) {
            r503_led(R503_LED_FLASHING, 150, R503_LED_RED, 3);
            result->status = R503_STATUS_COMM_ERROR;
            ESP_LOGW(TAG, "AutoIdentify: no response (attempt %d)", read_count);
            return ESP_OK;
        }

        uint8_t resp_code = ack[0];
        ESP_LOGI(TAG, "AutoIdentify response[%d]: 0x%02X (ack_len=%u, bytes: %02X %02X %02X %02X %02X)",
                 read_count, resp_code, (unsigned)ack_len,
                 ack_len > 1 ? ack[1] : 0, ack_len > 2 ? ack[2] : 0,
                 ack_len > 3 ? ack[3] : 0, ack_len > 4 ? ack[4] : 0,
                 ack_len > 5 ? ack[5] : 0);

        if (resp_code != R503_RESP_OK) {
            // Definitive error: no finger, no match, or other error
            if (resp_code == R503_RESP_NO_FINGER) {
                result->status = R503_STATUS_NO_FINGER;
                r503_led(R503_LED_FLASHING, 150, R503_LED_BLUE, 3);
                vTaskDelay(pdMS_TO_TICKS(1000));
                r503_led(R503_LED_FLASHING, 150, R503_LED_YELLOW, 3);
            } else if (resp_code == R503_RESP_NO_MATCH || resp_code == R503_RESP_NOT_FOUND) {
                result->status = R503_STATUS_NO_MATCH;
                r503_led(R503_LED_FLASHING, 150, R503_LED_BLUE, 3);
                vTaskDelay(pdMS_TO_TICKS(1000));
                r503_led(R503_LED_FLASHING, 150, R503_LED_RED, 3);
            } else {
                result->status = R503_STATUS_SENSOR_ERROR;
                r503_led(R503_LED_FLASHING, 150, R503_LED_RED, 3);
                ESP_LOGW(TAG, "AutoIdentify error code: 0x%02X", resp_code);
            }
            return ESP_OK;
        }

        // confirm=0x00: this step succeeded with no match data yet (steps 1 or 2).
        // Confirmed 6-byte response format from R503-M22 manual:
        //   [0]=confirm, [1]=step_number (1,2,3), [2]=id_H, [3]=id_L, [4]=score_H, [5]=score_L
        // Steps 1 and 2 have id+score all zero. Step 3 match has non-zero id or score.
        // Check bytes[2..5] only — byte[1] is the step counter (always 1, 2, or 3, never zero).
        bool has_match_data = false;
        if (ack_len >= 6) {
            for (int i = 2; i < (int)ack_len; i++) {
                if (ack[i] != 0) {
                    has_match_data = true;
                    break;
                }
            }
        }

        if (has_match_data) {
            // Always 6-byte AutoIdentify format: [confirm][attempt][id_H][id_L][score_H][score_L]
            int fp_id = (int)(((uint16_t)ack[2] << 8) | ack[3]);
            int score = (int)(((uint16_t)ack[4] << 8) | ack[5]);
            result->matched = true;
            result->matched_template_id = fp_id;
            result->confidence = (uint16_t)score;
            result->status = R503_STATUS_OK;
            r503_led(R503_LED_FLASHING, 150, R503_LED_GREEN, 3);
            ESP_LOGI(TAG, "AutoIdentify match: template_id=%d confidence=%d", fp_id, score);
            return ESP_OK;
        }

        // Steps 1 or 2 passed — read next step response
        ESP_LOGI(TAG, "AutoIdentify: step %d passed, waiting for next...", read_count + 1);
        ack_len = 0;
        err = read_r503_ack(ack, sizeof(ack), &ack_len);
    }

    // Exhausted retry limit — treat as no match
    result->status = R503_STATUS_NO_MATCH;
    r503_led(R503_LED_FLASHING, 150, R503_LED_RED, 3);
    return ESP_OK;
}

esp_err_t r503_device_enroll(int userid, int *out_template_id, r503_status_t *out_status)
{
    (void)userid; // AutoEnroll uses sensor auto-assign (0xC8); userid is only for caller bookkeeping
    if (!s_r503_ready) {
        esp_err_t init_err = r503_device_init();
        if (init_err != ESP_OK) {
            if (out_status) *out_status = R503_STATUS_COMM_ERROR;
            return init_err;
        }
    }

    if (out_template_id) *out_template_id = -1;
    if (out_status)      *out_status = R503_STATUS_SENSOR_ERROR;

    // AutoEnroll (0x31): the sensor handles all 6 image collections, feature generation,
    // duplicate check, template merging, and storage entirely internally.
    //
    // LED sequence (driven by R503 hardware — no explicit LED commands needed):
    //   For each of 6 scans: BLUE blink (collecting) → YELLOW (scan OK) → WHITE blink (lift finger)
    //   Final: GREEN blink (success) or RED blink (failure)
    //
    // Command params:
    //   0xC8 = auto-assign ModelID (sensor picks next free slot, returns it in final ACK)
    //   0x00 = no overwrite of existing template at same ID
    //   0x00 = reject duplicate fingerprints (same finger already enrolled → confirm 0x27)
    //   0x00 = no per-step ACKs (LED handles UX; one final ACK only)
    //   0x01 = finger lift required between each of the 6 image collections
    //
    // Final ACK format (14 bytes total, payload = 3 bytes):
    //   ack[0] = confirm code (0x00=OK, see codes below)
    //   ack[1] = step number  (0x0F=15 = final step for AutoEnroll)
    //   ack[2] = ModelID      (1 byte, 0x00–0xFF, auto-assigned slot)
    // Example success ACK: EF 01 FF FF FF FF 07 00 05 00 0F 00 00 1B (ModelID=0)
    // Confirm codes: 0x00=OK, 0x01=fail, 0x07=generate fail, 0x0A=merge fail,
    //                0x0B=ID out of range, 0x1F=library full, 0x22=template empty,
    //                0x26=timeout, 0x27=duplicate
    const uint8_t cmd[] = {R503_CMD_AUTO_ENROLL, 0xC8, 0x00, 0x00, 0x00, 0x01};
    uint8_t ack[8];
    size_t ack_len = 0;
    esp_err_t err = send_r503_command_timed(cmd, sizeof(cmd), ack, sizeof(ack), &ack_len,
                                            R503_AUTO_ENROLL_TIMEOUT_MS);
    if (err != ESP_OK || ack_len < 1) {
        if (out_status) *out_status = (err == ESP_ERR_TIMEOUT) ? R503_STATUS_TIMEOUT : R503_STATUS_COMM_ERROR;
        return ESP_OK;
    }

    uint8_t confirm = ack[0];
    ESP_LOGI(TAG, "AutoEnroll confirm=0x%02X ack_len=%u", confirm, (unsigned)ack_len);

    if (confirm != R503_RESP_OK) {
        r503_status_t mapped = map_response_code(confirm);
        ESP_LOGW(TAG, "AutoEnroll failed: confirm=0x%02X (%s)", confirm, r503_status_to_string(mapped));
        if (out_status) *out_status = mapped;
        return ESP_OK;
    }

    if (ack_len < 3) {
        ESP_LOGW(TAG, "AutoEnroll: short ACK (len=%u), cannot read ModelID", (unsigned)ack_len);
        if (out_status) *out_status = R503_STATUS_COMM_ERROR;
        return ESP_OK;
    }

    // ACK: [confirm=0x00][step=0x0F][model_id (1 byte)]
    int model_id = (int)ack[2];
    ESP_LOGI(TAG, "AutoEnroll success: step=0x%02X ModelID=%d", ack[1], model_id);
    if (out_template_id) *out_template_id = model_id;
    if (out_status)      *out_status = R503_STATUS_OK;
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
