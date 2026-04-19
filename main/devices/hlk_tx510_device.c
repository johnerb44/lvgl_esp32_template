#include "devices/hlk_tx510_device.h"
#include "comm/sc16is752_transport.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "HLK_TX510";
static bool s_hlk_ready = false;

// Packet framing constants
#define HLK_SYNC_0      0xEF
#define HLK_SYNC_1      0xAA
#define HLK_REPLY_ID    0x00

// Command codes
#define HLK_CMD_IDENTIFY    0x12
#define HLK_CMD_REGISTER    0x13
#define HLK_CMD_DELETE      0x20

// Maximum RX buffer — largest expected response is ~32 bytes
#define HLK_RX_BUF_MAX  48

/**
 * Build a command packet:  EF AA | MsgID | Size[4 big-endian] | Data[N] | Checksum
 * Checksum = (sum of all bytes after EF AA) & 0xFF
 */
static void build_cmd(uint8_t msgid, const uint8_t *data, uint32_t dlen,
                      uint8_t *out, size_t *out_len)
{
    out[0] = HLK_SYNC_0;
    out[1] = HLK_SYNC_1;
    out[2] = msgid;
    out[3] = (uint8_t)((dlen >> 24) & 0xFF);
    out[4] = (uint8_t)((dlen >> 16) & 0xFF);
    out[5] = (uint8_t)((dlen >>  8) & 0xFF);
    out[6] = (uint8_t)( dlen        & 0xFF);

    uint8_t cksum = msgid + out[3] + out[4] + out[5] + out[6];
    for (uint32_t i = 0; i < dlen; i++) {
        out[7 + i] = data[i];
        cksum += data[i];
    }
    out[7 + dlen] = cksum;
    *out_len = 8 + dlen;
}

/**
 * Parse a response packet.
 * Format: EF AA | 0x00 | Size[4 big-endian] | MsgID | Result | Data[N] | Checksum
 * Size = MsgID(1) + Result(1) + DataBytes(N) so actual data bytes = Size - 2
 */
static esp_err_t parse_response(const uint8_t *buf, size_t len, uint8_t expected_msgid,
                                 uint8_t *out_result,
                                 const uint8_t **out_data, size_t *out_dlen)
{
    // Minimum: 2(sync) + 1(reply_id) + 4(size) + 1(msgid) + 1(result) + 1(cksum) = 10
    if (len < 10) {
        ESP_LOGW(TAG, "Response too short: %u bytes", (unsigned)len);
        return ESP_ERR_INVALID_SIZE;
    }
    if (buf[0] != HLK_SYNC_0 || buf[1] != HLK_SYNC_1 || buf[2] != HLK_REPLY_ID) {
        ESP_LOGW(TAG, "Bad sync/reply header: %02X %02X %02X", buf[0], buf[1], buf[2]);
        return ESP_ERR_INVALID_RESPONSE;
    }

    uint32_t size = ((uint32_t)buf[3] << 24) | ((uint32_t)buf[4] << 16) |
                    ((uint32_t)buf[5] <<  8) |  (uint32_t)buf[6];
    if (size < 2) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    // Total packet = 2(sync) + 1(reply_id) + 4(size_field) + size(payload) + 1(cksum) = 8+size
    if (len < 8 + size) {
        ESP_LOGW(TAG, "Response truncated: got %u, need %lu", (unsigned)len, (unsigned long)(8+size));
        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t msgid = buf[7];
    if (msgid != expected_msgid) {
        ESP_LOGW(TAG, "MsgID mismatch: expected 0x%02X got 0x%02X", expected_msgid, msgid);
        return ESP_ERR_INVALID_RESPONSE;
    }

    if (out_result) *out_result = buf[8];
    if (out_data)   *out_data   = &buf[9];
    if (out_dlen)   *out_dlen   = size - 2;  // subtract MsgID(1) + Result(1)
    return ESP_OK;
}

const char *hlk_tx510_status_to_string(hlk_tx510_status_t status)
{
    switch (status) {
        case HLK_TX510_STATUS_OK:          return "ok";
        case HLK_TX510_STATUS_NO_FACE:     return "no_face";
        case HLK_TX510_STATUS_POSE_ERROR:  return "pose_error";
        case HLK_TX510_STATUS_2D_LIVENESS: return "liveness_2d";
        case HLK_TX510_STATUS_3D_LIVENESS: return "liveness_3d";
        case HLK_TX510_STATUS_NO_MATCH:    return "no_match";
        case HLK_TX510_STATUS_DUPLICATE:   return "duplicate";
        case HLK_TX510_STATUS_SAVE_ERROR:  return "save_error";
        case HLK_TX510_STATUS_COMM_ERROR:  return "comm_error";
        case HLK_TX510_STATUS_TIMEOUT:     return "timeout";
        default:                           return "unknown";
    }
}

esp_err_t hlk_tx510_device_init(void)
{
#if !CONFIG_LOCKBOX_INTEGRATION_ENABLE || !CONFIG_LOCKBOX_FEATURE_HLK_TX510
    s_hlk_ready = false;
    return ESP_ERR_NOT_SUPPORTED;
#else
    // Transport init configures both CH_A (57600, R503) and CH_B (115200, HLK-TX510).
    // It is idempotent — safe to call if R503 already initialized it.
    esp_err_t err = sc16is752_transport_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Transport init failed: %s", esp_err_to_name(err));
        s_hlk_ready = false;
        return err;
    }
    s_hlk_ready = true;
    // Note: The HLK-TX510 auto-sends one 0x12 response on power-up. The exchange()
    // pre-drain will flush this stale packet before the first command is sent.
    ESP_LOGI(TAG, "HLK-TX510 device ready on SC16IS752 CH_B at 115200 baud");
    return ESP_OK;
#endif
}

esp_err_t hlk_tx510_device_match(hlk_tx510_match_result_t *result)
{
    if (result == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    result->status = HLK_TX510_STATUS_COMM_ERROR;
    result->matched = false;
    result->matched_userid = -1;
    result->matched_face_id = -1;

#if !CONFIG_LOCKBOX_INTEGRATION_ENABLE || !CONFIG_LOCKBOX_FEATURE_HLK_TX510
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (!s_hlk_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t cmd[8];
    size_t cmd_len = 0;
    build_cmd(HLK_CMD_IDENTIFY, NULL, 0, cmd, &cmd_len);

    uint8_t rx[HLK_RX_BUF_MAX];
    size_t rx_len = 0;
    esp_err_t err = sc16is752_transport_exchange(LOCKBOX_COMM_CHANNEL_FACE,
                                                 cmd, cmd_len,
                                                 rx, sizeof(rx), &rx_len);
    if (err != ESP_OK) {
        if (err == ESP_ERR_NOT_FOUND) {
            result->status = HLK_TX510_STATUS_TIMEOUT;
        }
        ESP_LOGW(TAG, "Match exchange failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t resp_result = 0;
    const uint8_t *data = NULL;
    size_t dlen = 0;
    err = parse_response(rx, rx_len, HLK_CMD_IDENTIFY, &resp_result, &data, &dlen);
    if (err != ESP_OK) {
        result->status = HLK_TX510_STATUS_COMM_ERROR;
        return err;
    }

    result->status = (hlk_tx510_status_t)resp_result;
    if (resp_result == 0x00 && dlen >= 2 && data != NULL) {
        result->matched = true;
        result->matched_face_id = ((int)data[0] << 8) | data[1];
        ESP_LOGI(TAG, "Face matched: face_id=%d", result->matched_face_id);
    } else {
        result->matched = false;
        ESP_LOGI(TAG, "Face not matched: status=0x%02X (%s)", resp_result,
                 hlk_tx510_status_to_string(result->status));
    }
    return ESP_OK;
#endif
}

esp_err_t hlk_tx510_device_enroll(int userid, int *out_face_id)
{
    (void)userid;
    if (out_face_id) *out_face_id = -1;

#if !CONFIG_LOCKBOX_INTEGRATION_ENABLE || !CONFIG_LOCKBOX_FEATURE_HLK_TX510
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (!s_hlk_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t cmd[8];
    size_t cmd_len = 0;
    build_cmd(HLK_CMD_REGISTER, NULL, 0, cmd, &cmd_len);

    uint8_t rx[HLK_RX_BUF_MAX];
    size_t rx_len = 0;
    esp_err_t err = sc16is752_transport_exchange(LOCKBOX_COMM_CHANNEL_FACE,
                                                 cmd, cmd_len,
                                                 rx, sizeof(rx), &rx_len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Enroll exchange failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t resp_result = 0;
    const uint8_t *data = NULL;
    size_t dlen = 0;
    err = parse_response(rx, rx_len, HLK_CMD_REGISTER, &resp_result, &data, &dlen);
    if (err != ESP_OK) {
        return err;
    }

    if (resp_result != 0x00) {
        ESP_LOGW(TAG, "Enroll failed: status=0x%02X (%s)", resp_result,
                 hlk_tx510_status_to_string((hlk_tx510_status_t)resp_result));
        return ESP_FAIL;
    }
    if (dlen >= 2 && data != NULL && out_face_id) {
        *out_face_id = ((int)data[0] << 8) | data[1];
        ESP_LOGI(TAG, "Enroll succeeded: face_id=%d", *out_face_id);
    }
    return ESP_OK;
#endif
}

esp_err_t hlk_tx510_device_delete_face(int face_id)
{
#if !CONFIG_LOCKBOX_INTEGRATION_ENABLE || !CONFIG_LOCKBOX_FEATURE_HLK_TX510
    return ESP_ERR_NOT_SUPPORTED;
#else
    if (!s_hlk_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t face_data[2] = { (uint8_t)((face_id >> 8) & 0xFF), (uint8_t)(face_id & 0xFF) };
    uint8_t cmd[10];
    size_t cmd_len = 0;
    build_cmd(HLK_CMD_DELETE, face_data, 2, cmd, &cmd_len);

    uint8_t rx[HLK_RX_BUF_MAX];
    size_t rx_len = 0;
    esp_err_t err = sc16is752_transport_exchange(LOCKBOX_COMM_CHANNEL_FACE,
                                                 cmd, cmd_len,
                                                 rx, sizeof(rx), &rx_len);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Delete exchange failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t resp_result = 0;
    const uint8_t *data = NULL;
    size_t dlen = 0;
    err = parse_response(rx, rx_len, HLK_CMD_DELETE, &resp_result, &data, &dlen);
    if (err != ESP_OK) {
        return err;
    }

    if (resp_result != 0x00) {
        ESP_LOGW(TAG, "Delete failed: status=0x%02X", resp_result);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Deleted face_id=%d", face_id);
    return ESP_OK;
#endif
}

