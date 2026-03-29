#pragma once

#include "esp_err.h"
#include "driver/i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

#define I2C_MANAGER_PORT I2C_NUM_0
#define I2C_MANAGER_SDA_GPIO GPIO_NUM_22
#define I2C_MANAGER_SCL_GPIO GPIO_NUM_21
#define I2C_MANAGER_FREQUENCY_HZ 100000

esp_err_t i2c_manager_init(void);
i2c_port_t i2c_manager_get_port(void);

#ifdef __cplusplus
}
#endif
