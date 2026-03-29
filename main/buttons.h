#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUTTONS_ACTIVE_LEVEL 0
#define BTN1_GPIO GPIO_NUM_36
#define BTN2_GPIO GPIO_NUM_32
#define BTN3_GPIO GPIO_NUM_33
#define BTN4_GPIO GPIO_NUM_25

typedef enum {
    BUTTON_ID_1 = 0,
    BUTTON_ID_2,
    BUTTON_ID_3,
    BUTTON_ID_4,
    BUTTON_ID_COUNT,
} button_id_t;

esp_err_t buttons_init(void);
esp_err_t buttons_read(button_id_t button, bool *pressed);
const char *buttons_name(button_id_t button);

#ifdef __cplusplus
}
#endif
