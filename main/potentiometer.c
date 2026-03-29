#include "potentiometer.h"

#include <stdbool.h>

#include "esp_log.h"

static const char *TAG = "potentiometer";
static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static bool s_potentiometer_initialized = false;
static uint8_t s_last_percent = 0;

esp_err_t potentiometer_init(void)
{
    if (s_potentiometer_initialized) {
        return ESP_OK;
    }

    const adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = POTENTIOMETER_ADC_UNIT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(err));
        return err;
    }

    const adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = POTENTIOMETER_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    err = adc_oneshot_config_channel(s_adc_handle, POTENTIOMETER_ADC_CHANNEL, &chan_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(err));
        adc_oneshot_del_unit(s_adc_handle);
        s_adc_handle = NULL;
        return err;
    }

    s_potentiometer_initialized = true;
    ESP_LOGI(TAG, "Potentiometer initialized on GPIO %d", POTENTIOMETER_GPIO);
    return ESP_OK;
}

esp_err_t potentiometer_read_raw(int *raw_value)
{
    if (raw_value == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_potentiometer_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return adc_oneshot_read(s_adc_handle, POTENTIOMETER_ADC_CHANNEL, raw_value);
}

esp_err_t potentiometer_read_percent(uint8_t *percent)
{
    if (percent == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int raw_sum = 0;
    for (int i = 0; i < POTENTIOMETER_AVG_SAMPLES; ++i) {
        int raw_value = 0;
        esp_err_t err = potentiometer_read_raw(&raw_value);
        if (err != ESP_OK) {
            return err;
        }
        raw_sum += raw_value;
    }

    int raw_value = raw_sum / POTENTIOMETER_AVG_SAMPLES;

    if (raw_value < POTENTIOMETER_RAW_MIN) {
        raw_value = POTENTIOMETER_RAW_MIN;
    }

    if (raw_value > POTENTIOMETER_RAW_MAX) {
        raw_value = POTENTIOMETER_RAW_MAX;
    }

    int range = POTENTIOMETER_RAW_MAX - POTENTIOMETER_RAW_MIN;
    if (range <= 0) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t mapped_percent = (uint8_t)(
        ((raw_value - POTENTIOMETER_RAW_MIN) * POTENTIOMETER_MAX_PERCENT) / range
    );

    if (mapped_percent < POTENTIOMETER_PERCENT_STEP) {
        mapped_percent = 0;
    } else if (mapped_percent > (100 - POTENTIOMETER_PERCENT_STEP)) {
        mapped_percent = 100;
    } else {
        mapped_percent = (uint8_t)(
            (mapped_percent / POTENTIOMETER_PERCENT_STEP) * POTENTIOMETER_PERCENT_STEP
        );
    }

    s_last_percent = mapped_percent;
    *percent = s_last_percent;
    return ESP_OK;
}
