#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POTENTIOMETER_ADC_UNIT ADC_UNIT_1
#define POTENTIOMETER_ADC_CHANNEL ADC_CHANNEL_6
#define POTENTIOMETER_GPIO GPIO_NUM_34
#define POTENTIOMETER_ATTEN ADC_ATTEN_DB_12
#define POTENTIOMETER_RAW_MIN 200
#define POTENTIOMETER_RAW_MAX 3800
#define POTENTIOMETER_AVG_SAMPLES 16
#define POTENTIOMETER_PERCENT_STEP 2
#define POTENTIOMETER_MIN_PERCENT 0
#define POTENTIOMETER_MAX_PERCENT 100

esp_err_t potentiometer_init(void);
esp_err_t potentiometer_read_raw(int *raw_value);
esp_err_t potentiometer_read_percent(uint8_t *percent);

#ifdef __cplusplus
}
#endif
