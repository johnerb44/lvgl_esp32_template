#ifndef PCA9685_DEVICE_H
#define PCA9685_DEVICE_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOCK_SERVO_POSITION_LOCKED = 0,
    LOCK_SERVO_POSITION_UNLOCKED = 1,
} lock_servo_position_t;

esp_err_t pca9685_device_init(void);
esp_err_t pca9685_device_set_lock_position(lock_servo_position_t position);

#ifdef __cplusplus
}
#endif

#endif // PCA9685_DEVICE_H
