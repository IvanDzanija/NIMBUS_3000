#pragma once

#include <stdbool.h>

#include "APDS_9960.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t sensors_init(void);
esp_err_t sensors_init_all(void);
bool sensors_get_latest_lux(double *lux);
esp_err_t sensors_init_apds9960(void);
esp_err_t sensors_read_apds9960_color(apds9960_color_data_t *color);
esp_err_t sensors_read_apds9960_gesture(apds9960_gesture_t *gesture);
esp_err_t sensors_read_veml7700_lux(double *lux);
const char *sensors_detect_apds9960_color_name(const apds9960_color_data_t *color);

#ifdef __cplusplus
}
#endif
