#include "sensors.h"

#include "esp_log.h"

#include "APDS_9960.h"
#include "i2c_manager.h"
#include "veml7700.h"

static const char *TAG = "sensors";
static veml7700_handle_t s_veml7700 = NULL;
static apds9960_t s_apds9960;

#define COLOR_MIN_VALID_CLEAR 150
#define COLOR_CHANNEL_MARGIN 40

esp_err_t sensors_init(void)
{
    esp_err_t err = i2c_manager_init();
    if (err != ESP_OK) {
        return err;
    }

    err = veml7700_initialize(&s_veml7700, i2c_manager_get_port());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize VEML7700: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t sensors_init_all(void)
{
    esp_err_t err = sensors_init();
    if (err != ESP_OK) {
        return err;
    }

    return sensors_init_apds9960();
}

bool sensors_get_latest_lux(double *lux)
{
    return (sensors_read_veml7700_lux(lux) == ESP_OK);
}

esp_err_t sensors_init_apds9960(void)
{
    esp_err_t err = i2c_manager_init();
    if (err != ESP_OK) {
        return err;
    }

    err = apds9960_init(&s_apds9960, i2c_manager_get_port());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize APDS-9960: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

esp_err_t sensors_read_apds9960_color(apds9960_color_data_t *color)
{
    return apds9960_read_color(&s_apds9960, color);
}

esp_err_t sensors_read_apds9960_gesture(apds9960_gesture_t *gesture)
{
    return apds9960_read_gesture(&s_apds9960, gesture);
}

esp_err_t sensors_read_veml7700_lux(double *lux)
{
    if (s_veml7700 == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    return veml7700_read_als_lux_auto(s_veml7700, lux);
}

const char *sensors_detect_apds9960_color_name(const apds9960_color_data_t *color)
{
    if (color == NULL) {
        return "UNKNOWN";
    }

    if (color->clear < COLOR_MIN_VALID_CLEAR) {
        return "DARK";
    }

    uint16_t r = color->red;
    uint16_t g = color->green;
    uint16_t b = color->blue;

    bool r_close_g = (r > g ? r - g : g - r) < COLOR_CHANNEL_MARGIN;
    bool r_close_b = (r > b ? r - b : b - r) < COLOR_CHANNEL_MARGIN;
    bool g_close_b = (g > b ? g - b : b - g) < COLOR_CHANNEL_MARGIN;

    if (r_close_g && r_close_b && g_close_b) {
        return "WHITE";
    }

    if (r_close_g && (r > b)) {
        return "YELLOW";
    }

    if (r_close_b && (r > g)) {
        return "MAGENTA";
    }

    if (g_close_b && (g > r)) {
        return "CYAN";
    }

    if ((r > g) && (r > b)) {
        if (g > b) {
            return "ORANGE";
        }
        return "RED";
    }

    if ((g > r) && (g > b)) {
        return "GREEN";
    }

    if ((b > r) && (b > g)) {
        return "BLUE";
    }

    return "UNKNOWN";
}
