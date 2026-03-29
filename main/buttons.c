#include "buttons.h"

#include <stddef.h>

#include "esp_log.h"

typedef struct {
    button_id_t id;
    gpio_num_t gpio;
    const char *name;
} button_desc_t;

static const char *TAG = "buttons";
static bool s_buttons_initialized = false;

static const button_desc_t s_buttons[BUTTON_ID_COUNT] = {
    {BUTTON_ID_1, BTN1_GPIO, "BTN1"},
    {BUTTON_ID_2, BTN2_GPIO, "BTN2"},
    {BUTTON_ID_3, BTN3_GPIO, "BTN3"},
    {BUTTON_ID_4, BTN4_GPIO, "BTN4"},
};

static const button_desc_t *buttons_get_desc(button_id_t button)
{
    if ((int)button < 0 || button >= BUTTON_ID_COUNT) {
        return NULL;
    }

    return &s_buttons[button];
}

esp_err_t buttons_init(void)
{
    if (s_buttons_initialized) {
        return ESP_OK;
    }

    for (size_t i = 0; i < BUTTON_ID_COUNT; ++i) {
        gpio_config_t cfg = {
            .pin_bit_mask = (1ULL << s_buttons[i].gpio),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };

        /* GPIO36 is input-only and has no internal pulls. */
        if (s_buttons[i].gpio != GPIO_NUM_36) {
            cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        }

        esp_err_t err = gpio_config(&cfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "gpio_config failed for %s: %s",
                s_buttons[i].name, esp_err_to_name(err));
            return err;
        }
    }

    s_buttons_initialized = true;
    ESP_LOGI(TAG, "Buttons initialized");
    return ESP_OK;
}

esp_err_t buttons_read(button_id_t button, bool *pressed)
{
    if (pressed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_buttons_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    const button_desc_t *desc = buttons_get_desc(button);
    if (desc == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    int level = gpio_get_level(desc->gpio);
    *pressed = (level == BUTTONS_ACTIVE_LEVEL);
    return ESP_OK;
}

const char *buttons_name(button_id_t button)
{
    const button_desc_t *desc = buttons_get_desc(button);
    return (desc != NULL) ? desc->name : "UNKNOWN";
}
