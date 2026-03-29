#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APDS9960_I2C_ADDR UINT8_C(0x39)
#define APDS9960_ID_VALUE UINT8_C(0xAB)

typedef struct {
    i2c_port_t port;
    uint8_t address;
} apds9960_t;

typedef struct {
    uint16_t clear;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
} apds9960_color_data_t;

typedef enum {
    APDS9960_GESTURE_NONE = 0,
    APDS9960_GESTURE_UP,
    APDS9960_GESTURE_DOWN,
    APDS9960_GESTURE_LEFT,
    APDS9960_GESTURE_RIGHT,
    APDS9960_GESTURE_NEAR,
    APDS9960_GESTURE_FAR,
} apds9960_gesture_t;

esp_err_t apds9960_init(apds9960_t *dev, i2c_port_t port);
esp_err_t apds9960_read_color(const apds9960_t *dev, apds9960_color_data_t *color);
esp_err_t apds9960_read_gesture(const apds9960_t *dev, apds9960_gesture_t *gesture);
const char *apds9960_gesture_to_string(apds9960_gesture_t gesture);

#ifdef __cplusplus
}
#endif
