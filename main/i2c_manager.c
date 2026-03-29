#include "i2c_manager.h"

#include <stdbool.h>

#include "esp_log.h"

static const char *TAG = "i2c_manager";
static bool s_i2c_initialized = false;

esp_err_t i2c_manager_init(void)
{
    if (s_i2c_initialized) {
        return ESP_OK;
    }

    const i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MANAGER_SDA_GPIO,
        .scl_io_num = I2C_MANAGER_SCL_GPIO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MANAGER_FREQUENCY_HZ,
        .clk_flags = 0,
    };

    esp_err_t err = i2c_param_config(I2C_MANAGER_PORT, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_param_config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_MANAGER_PORT, config.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_driver_install failed: %s", esp_err_to_name(err));
        return err;
    }

    s_i2c_initialized = true;
    ESP_LOGI(TAG, "I2C initialized on port %d, SDA=%d, SCL=%d",
        I2C_MANAGER_PORT, I2C_MANAGER_SDA_GPIO, I2C_MANAGER_SCL_GPIO);

    return ESP_OK;
}

i2c_port_t i2c_manager_get_port(void)
{
    return I2C_MANAGER_PORT;
}
