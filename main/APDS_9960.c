#include "APDS_9960.h"

#include <stddef.h>

#include "freertos/FreeRTOS.h"

#include "esp_log.h"

#define APDS9960_REG_ENABLE 0x80
#define APDS9960_REG_ATIME 0x81
#define APDS9960_REG_WTIME 0x83
#define APDS9960_REG_CDATAL 0x94
#define APDS9960_REG_CDATAH 0x95
#define APDS9960_REG_RDATAL 0x96
#define APDS9960_REG_RDATAH 0x97
#define APDS9960_REG_GDATAL 0x98
#define APDS9960_REG_GDATAH 0x99
#define APDS9960_REG_BDATAL 0x9A
#define APDS9960_REG_BDATAH 0x9B
#define APDS9960_REG_PPULSE 0x8E
#define APDS9960_REG_CONTROL 0x8F
#define APDS9960_REG_CONFIG2 0x90
#define APDS9960_REG_ID 0x92
#define APDS9960_REG_GSTATUS 0xAF
#define APDS9960_REG_GCONF4 0xAB
#define APDS9960_REG_GFLVL 0xAE
#define APDS9960_REG_GPENTH 0xA0
#define APDS9960_REG_GEXTH 0xA1
#define APDS9960_REG_GCONF1 0xA2
#define APDS9960_REG_GCONF2 0xA3
#define APDS9960_REG_GOFFSET_U 0xA4
#define APDS9960_REG_GOFFSET_D 0xA5
#define APDS9960_REG_GPULSE 0xA6
#define APDS9960_REG_GOFFSET_L 0xA7
#define APDS9960_REG_GOFFSET_R 0xA9
#define APDS9960_REG_GCONF3 0xAA
#define APDS9960_REG_GFIFO_U 0xFC

#define APDS9960_ENABLE_PON 0x01
#define APDS9960_ENABLE_AEN 0x02
#define APDS9960_ENABLE_PEN 0x04
#define APDS9960_ENABLE_WEN 0x08
#define APDS9960_ENABLE_GEN 0x40

#define APDS9960_GSTATUS_GVALID 0x01

#define APDS9960_CONFIG2_LEDBOOST_100 0x00
#define APDS9960_CONFIG2_LEDBOOST_150 0x10
#define APDS9960_CONFIG2_LEDBOOST_200 0x20
#define APDS9960_CONFIG2_LEDBOOST_300 0x30

#define APDS9960_GCONF1_GFIFOTH_1_DATASET 0x00
#define APDS9960_GCONF1_GFIFOTH_4_DATASETS 0x40
#define APDS9960_GCONF1_GFIFOTH_8_DATASETS 0x80
#define APDS9960_GCONF1_GFIFOTH_16_DATASETS 0xC0

#define APDS9960_GCONF2_GGAIN_1X 0x00
#define APDS9960_GCONF2_GGAIN_2X 0x20
#define APDS9960_GCONF2_GGAIN_4X 0x40
#define APDS9960_GCONF2_GGAIN_8X 0x60
#define APDS9960_GCONF2_GLDRIVE_100MA 0x00
#define APDS9960_GCONF2_GLDRIVE_50MA 0x08
#define APDS9960_GCONF2_GLDRIVE_25MA 0x10
#define APDS9960_GCONF2_GLDRIVE_12_5MA 0x18
#define APDS9960_GCONF2_GWTIME_0MS 0x00
#define APDS9960_GCONF2_GWTIME_2_8MS 0x01
#define APDS9960_GCONF2_GWTIME_5_6MS 0x02
#define APDS9960_GCONF2_GWTIME_8_4MS 0x03
#define APDS9960_GCONF2_GWTIME_14MS 0x04
#define APDS9960_GCONF2_GWTIME_22_4MS 0x05
#define APDS9960_GCONF2_GWTIME_30_8MS 0x06
#define APDS9960_GCONF2_GWTIME_39_2MS 0x07

#define APDS9960_GPULSE_4US 0x00
#define APDS9960_GPULSE_8US 0x40
#define APDS9960_GPULSE_16US 0x80
#define APDS9960_GPULSE_32US 0xC0

#define APDS9960_GCONF3_GDIMS_ALL 0x00
#define APDS9960_GCONF4_GMODE 0x01
#define APDS9960_GCONF4_GIEN 0x02

static const char *TAG = "APDS9960";
static const uint8_t APDS9960_GESTURE_FRAME_THRESHOLD = 24;
static const uint8_t APDS9960_GESTURE_SWIPE_THRESHOLD = 16;

static esp_err_t apds9960_write_u8(const apds9960_t *dev, uint8_t reg, uint8_t value)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t payload[2] = {reg, value};
    return i2c_master_write_to_device(dev->port, dev->address, payload, sizeof(payload),
        pdMS_TO_TICKS(1000));
}

static esp_err_t apds9960_read_u8(const apds9960_t *dev, uint8_t reg, uint8_t *value)
{
    if ((dev == NULL) || (value == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_write_read_device(dev->port, dev->address, &reg, sizeof(reg), value,
        sizeof(*value), pdMS_TO_TICKS(1000));
}

static esp_err_t apds9960_read_block(
    const apds9960_t *dev,
    uint8_t reg,
    uint8_t *data,
    size_t len)
{
    if ((dev == NULL) || (data == NULL) || (len == 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    return i2c_master_write_read_device(dev->port, dev->address, &reg, sizeof(reg), data, len,
        pdMS_TO_TICKS(1000));
}

static uint16_t apds9960_decode_u16(uint8_t low, uint8_t high)
{
    return (uint16_t)low | ((uint16_t)high << 8);
}

esp_err_t apds9960_init(apds9960_t *dev, i2c_port_t port)
{
    if (dev == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    dev->port = port;
    dev->address = APDS9960_I2C_ADDR;

    uint8_t chip_id = 0;
    esp_err_t err = apds9960_read_u8(dev, APDS9960_REG_ID, &chip_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read chip ID: %s", esp_err_to_name(err));
        return err;
    }

    if (chip_id != APDS9960_ID_VALUE) {
        ESP_LOGE(TAG, "Unexpected chip ID 0x%02X", chip_id);
        return ESP_ERR_INVALID_RESPONSE;
    }

    /* Power on first, then give the device time to leave sleep before gesture setup. */
    err = apds9960_write_u8(dev, APDS9960_REG_ENABLE, APDS9960_ENABLE_PON);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to power on APDS-9960: %s", esp_err_to_name(err));
        return err;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    const struct {
        uint8_t reg;
        uint8_t value;
    } init_sequence[] = {
        {APDS9960_REG_WTIME, 0xF6},
        {APDS9960_REG_PPULSE, 0x87},
        {APDS9960_REG_ATIME, 0xDB},
        {APDS9960_REG_CONTROL, 0x05},
        {APDS9960_REG_CONFIG2, APDS9960_CONFIG2_LEDBOOST_300},
        {APDS9960_REG_GPENTH, 20},
        {APDS9960_REG_GEXTH, 10},
        {APDS9960_REG_GCONF1, APDS9960_GCONF1_GFIFOTH_4_DATASETS},
        {APDS9960_REG_GCONF2, APDS9960_GCONF2_GGAIN_8X |
            APDS9960_GCONF2_GLDRIVE_100MA |
            APDS9960_GCONF2_GWTIME_30_8MS},
        {APDS9960_REG_GOFFSET_U, 0x00},
        {APDS9960_REG_GOFFSET_D, 0x00},
        {APDS9960_REG_GOFFSET_L, 0x00},
        {APDS9960_REG_GOFFSET_R, 0x00},
        {APDS9960_REG_GPULSE, APDS9960_GPULSE_32US | 0x0A},
        {APDS9960_REG_GCONF3, APDS9960_GCONF3_GDIMS_ALL},
        {APDS9960_REG_GCONF4, APDS9960_GCONF4_GIEN | APDS9960_GCONF4_GMODE},
        {APDS9960_REG_ENABLE, APDS9960_ENABLE_PON | APDS9960_ENABLE_AEN |
            APDS9960_ENABLE_PEN | APDS9960_ENABLE_WEN | APDS9960_ENABLE_GEN},
    };

    for (size_t i = 0; i < sizeof(init_sequence) / sizeof(init_sequence[0]); ++i) {
        err = apds9960_write_u8(dev, init_sequence[i].reg, init_sequence[i].value);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Init write failed at reg 0x%02X: %s",
                init_sequence[i].reg, esp_err_to_name(err));
            return err;
        }
    }

    ESP_LOGI(TAG, "APDS-9960 initialized");
    return ESP_OK;
}

esp_err_t apds9960_read_color(const apds9960_t *dev, apds9960_color_data_t *color)
{
    if ((dev == NULL) || (color == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[8] = {0};
    esp_err_t err = apds9960_read_block(dev, APDS9960_REG_CDATAL, raw, sizeof(raw));
    if (err != ESP_OK) {
        return err;
    }

    color->clear = apds9960_decode_u16(raw[0], raw[1]);
    color->red = apds9960_decode_u16(raw[2], raw[3]);
    color->green = apds9960_decode_u16(raw[4], raw[5]);
    color->blue = apds9960_decode_u16(raw[6], raw[7]);

    return ESP_OK;
}

esp_err_t apds9960_read_gesture(const apds9960_t *dev, apds9960_gesture_t *gesture)
{
    if ((dev == NULL) || (gesture == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    *gesture = APDS9960_GESTURE_NONE;

    uint8_t gstatus = 0;
    esp_err_t err = apds9960_read_u8(dev, APDS9960_REG_GSTATUS, &gstatus);
    if (err != ESP_OK) {
        return err;
    }

    if ((gstatus & APDS9960_GSTATUS_GVALID) == 0) {
        return ESP_OK;
    }

    uint8_t fifo_level = 0;
    err = apds9960_read_u8(dev, APDS9960_REG_GFLVL, &fifo_level);
    if (err != ESP_OK) {
        return err;
    }

    if (fifo_level == 0) {
        return ESP_OK;
    }

    uint8_t fifo[128] = {0};
    size_t bytes_to_read = (size_t)fifo_level * 4U;
    if (bytes_to_read > sizeof(fifo)) {
        bytes_to_read = sizeof(fifo);
    }

    err = apds9960_read_block(dev, APDS9960_REG_GFIFO_U, fifo, bytes_to_read);
    if (err != ESP_OK) {
        return err;
    }

    int u_first = 0;
    int d_first = 0;
    int l_first = 0;
    int r_first = 0;
    int u_last = 0;
    int d_last = 0;
    int l_last = 0;
    int r_last = 0;
    bool found_first = false;
    int valid_frames = 0;

    for (size_t i = 0; i + 3 < bytes_to_read; i += 4) {
        int u = fifo[i];
        int d = fifo[i + 1];
        int l = fifo[i + 2];
        int r = fifo[i + 3];

        if (u > APDS9960_GESTURE_FRAME_THRESHOLD ||
            d > APDS9960_GESTURE_FRAME_THRESHOLD ||
            l > APDS9960_GESTURE_FRAME_THRESHOLD ||
            r > APDS9960_GESTURE_FRAME_THRESHOLD) {
            if (!found_first) {
                u_first = u;
                d_first = d;
                l_first = l;
                r_first = r;
                found_first = true;
            }

            u_last = u;
            d_last = d;
            l_last = l;
            r_last = r;
            ++valid_frames;
        }
    }

    if (!found_first || valid_frames < 2) {
        return ESP_OK;
    }

    int ud_delta_first = u_first - d_first;
    int ud_delta_last = u_last - d_last;
    int lr_delta_first = l_first - r_first;
    int lr_delta_last = l_last - r_last;

    int ud_swipe = ud_delta_last - ud_delta_first;
    int lr_swipe = lr_delta_last - lr_delta_first;

    int abs_ud_swipe = (ud_swipe >= 0) ? ud_swipe : -ud_swipe;
    int abs_lr_swipe = (lr_swipe >= 0) ? lr_swipe : -lr_swipe;

    if ((abs_ud_swipe < APDS9960_GESTURE_SWIPE_THRESHOLD) &&
        (abs_lr_swipe < APDS9960_GESTURE_SWIPE_THRESHOLD)) {
        int first_sum = u_first + d_first + l_first + r_first;
        int last_sum = u_last + d_last + l_last + r_last;

        if (last_sum > first_sum + APDS9960_GESTURE_SWIPE_THRESHOLD) {
            *gesture = APDS9960_GESTURE_NEAR;
        } else if (first_sum > last_sum + APDS9960_GESTURE_SWIPE_THRESHOLD) {
            *gesture = APDS9960_GESTURE_FAR;
        }

        return ESP_OK;
    }

    if (abs_ud_swipe > abs_lr_swipe) {
        *gesture = (ud_swipe > 0) ? APDS9960_GESTURE_UP : APDS9960_GESTURE_DOWN;
    } else {
        *gesture = (lr_swipe > 0) ? APDS9960_GESTURE_LEFT : APDS9960_GESTURE_RIGHT;
    }

    return ESP_OK;
}

const char *apds9960_gesture_to_string(apds9960_gesture_t gesture)
{
    switch (gesture) {
        case APDS9960_GESTURE_UP:
            return "UP";
        case APDS9960_GESTURE_DOWN:
            return "DOWN";
        case APDS9960_GESTURE_LEFT:
            return "LEFT";
        case APDS9960_GESTURE_RIGHT:
            return "RIGHT";
        case APDS9960_GESTURE_NEAR:
            return "NEAR";
        case APDS9960_GESTURE_FAR:
            return "FAR";
        case APDS9960_GESTURE_NONE:
        default:
            return "NONE";
    }
}
