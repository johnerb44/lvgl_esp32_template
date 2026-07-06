#ifndef SC16IS752_TRANSPORT_H
#define SC16IS752_TRANSPORT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOCKBOX_COMM_CHANNEL_FINGERPRINT = 0,
    LOCKBOX_COMM_CHANNEL_FACE = 1,
    LOCKBOX_COMM_CHANNEL_STATUS = 2,
} lockbox_comm_channel_t;

esp_err_t sc16is752_transport_init(void);
bool sc16is752_transport_is_ready(void);
esp_err_t sc16is752_transport_probe(void);
esp_err_t sc16is752_transport_configure_channel(lockbox_comm_channel_t channel, uint32_t baud_rate);
esp_err_t sc16is752_transport_write(lockbox_comm_channel_t channel, const uint8_t *data, size_t len);
esp_err_t sc16is752_transport_read(lockbox_comm_channel_t channel, uint8_t *data, size_t max_len, size_t *out_len);
esp_err_t sc16is752_transport_exchange(lockbox_comm_channel_t channel,
                                       const uint8_t *tx_data, size_t tx_len,
                                       uint8_t *rx_data, size_t rx_max_len, size_t *out_rx_len);
esp_err_t sc16is752_transport_exchange_timed(lockbox_comm_channel_t channel,
                                             const uint8_t *tx_data, size_t tx_len,
                                             uint8_t *rx_data, size_t rx_max_len, size_t *out_rx_len,
                                             int first_byte_timeout_ms);
esp_err_t sc16is752_transport_gpio_init(uint8_t direction_mask, uint8_t initial_state);
esp_err_t sc16is752_transport_gpio_read(uint8_t *out_state);
esp_err_t sc16is752_transport_gpio_write(uint8_t state);

/* Mock R503 touch simulation for testing wake detection */
void sc16is752_transport_r503_set_mock_touch(bool touched);
bool sc16is752_transport_r503_get_mock_touch(void);

#ifdef __cplusplus
}
#endif

#endif // SC16IS752_TRANSPORT_H
