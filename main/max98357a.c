#include "max98357a.h"

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "esp_log.h"
#include "potentiometer.h"

static const char *TAG = "max98357a";
static bool s_max98357a_initialized = false;
static i2s_chan_handle_t s_tx_chan = NULL;
static uint8_t s_volume_percent = 100;
static bool s_alarm_force_max_volume = false;

extern const uint8_t dog_pcm_start[] asm("_binary_dog_pcm_start");
extern const uint8_t dog_pcm_end[] asm("_binary_dog_pcm_end");
extern const uint8_t cat_pcm_start[] asm("_binary_cat_pcm_start");
extern const uint8_t cat_pcm_end[] asm("_binary_cat_pcm_end");
extern const uint8_t cow_pcm_start[] asm("_binary_cow_pcm_start");
extern const uint8_t cow_pcm_end[] asm("_binary_cow_pcm_end");
extern const uint8_t pig_pcm_start[] asm("_binary_pig_pcm_start");
extern const uint8_t pig_pcm_end[] asm("_binary_pig_pcm_end");
extern const uint8_t horse_pcm_start[] asm("_binary_horse_pcm_start");
extern const uint8_t horse_pcm_end[] asm("_binary_horse_pcm_end");
extern const uint8_t sealion_pcm_start[] asm("_binary_sealion_v2_pcm_start");
extern const uint8_t sealion_pcm_end[] asm("_binary_sealion_v2_pcm_end");
extern const uint8_t message_pcm_start[] asm("_binary_message_pcm_start");
extern const uint8_t message_pcm_end[] asm("_binary_message_pcm_end");

esp_err_t max98357a_init(void)
{
    if (s_max98357a_initialized) {
        return ESP_OK;
    }

    const i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    esp_err_t err = i2s_new_channel(&chan_cfg, &s_tx_chan, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    const i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MAX98357A_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_MONO
        ),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MAX98357A_BCLK_GPIO,
            .ws = MAX98357A_LRC_GPIO,
            .dout = MAX98357A_DIN_GPIO,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(s_tx_chan, &std_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(err));
        i2s_del_channel(s_tx_chan);
        s_tx_chan = NULL;
        return err;
    }

    err = i2s_channel_enable(s_tx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(err));
        i2s_del_channel(s_tx_chan);
        s_tx_chan = NULL;
        return err;
    }

    s_max98357a_initialized = true;
    ESP_LOGI(TAG, "MAX98357A initialized on BCLK=%d LRC=%d DIN=%d",
        MAX98357A_BCLK_GPIO, MAX98357A_LRC_GPIO, MAX98357A_DIN_GPIO);

    return ESP_OK;
}

esp_err_t max98357a_deinit(void)
{
    if (!s_max98357a_initialized) {
        return ESP_OK;
    }

    esp_err_t err = i2s_channel_disable(s_tx_chan);
    if (err != ESP_OK) {
        return err;
    }

    err = i2s_del_channel(s_tx_chan);
    if (err == ESP_OK) {
        s_tx_chan = NULL;
        s_max98357a_initialized = false;
    }

    return err;
}

void max98357a_set_volume_percent(uint8_t percent)
{
    if (percent > 100U) {
        percent = 100U;
    }

    s_volume_percent = percent;
}

uint8_t max98357a_get_volume_percent(void)
{
    return s_volume_percent;
}

void max98357a_set_alarm_force_max_volume(bool enabled)
{
    s_alarm_force_max_volume = enabled;
}

esp_err_t max98357a_write_pcm_u8(const uint8_t *pcm_data, size_t pcm_length)
{
    if ((pcm_data == NULL) || (pcm_length == 0U)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_max98357a_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t *buffer = calloc(MAX98357A_BUFFER_SAMPLES, sizeof(uint16_t));
    if (buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    size_t offset = 0;
    esp_err_t err = ESP_OK;

    while (offset < pcm_length) {
        size_t samples_to_copy = MAX98357A_BUFFER_SAMPLES;
        if ((pcm_length - offset) < samples_to_copy) {
            samples_to_copy = pcm_length - offset;
        }

        uint8_t playback_volume_percent = s_alarm_force_max_volume ? 100U : s_volume_percent;
        if (!s_alarm_force_max_volume &&
            potentiometer_read_percent(&playback_volume_percent) == ESP_OK) {
            s_volume_percent = playback_volume_percent;
        }

        for (size_t i = 0; i < samples_to_copy; ++i) {
            int16_t signed_sample = ((int16_t)pcm_data[offset + i] - 128) << 8;
            signed_sample = (int16_t)((signed_sample * playback_volume_percent) / 100);
            buffer[i] = (uint16_t)signed_sample;
        }

        size_t bytes_written = 0;
        err = i2s_channel_write(
            s_tx_chan,
            buffer,
            samples_to_copy * sizeof(uint16_t),
            &bytes_written,
            1000
        );
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "i2s_channel_write failed: %s", esp_err_to_name(err));
            break;
        }

        offset += samples_to_copy;
    }

    free(buffer);
    return err;
}

esp_err_t max98357a_play_embedded_sound(max98357a_sound_t sound)
{
    switch (sound) {
        case MAX98357A_SOUND_DOG:
            return max98357a_write_pcm_u8(
                dog_pcm_start,
                (size_t)(dog_pcm_end - dog_pcm_start)
            );
        case MAX98357A_SOUND_CAT:
            return max98357a_write_pcm_u8(
                cat_pcm_start,
                (size_t)(cat_pcm_end - cat_pcm_start)
            );
        case MAX98357A_SOUND_COW:
            return max98357a_write_pcm_u8(
                cow_pcm_start,
                (size_t)(cow_pcm_end - cow_pcm_start)
            );
        case MAX98357A_SOUND_PIG:
            return max98357a_write_pcm_u8(
                pig_pcm_start,
                (size_t)(pig_pcm_end - pig_pcm_start)
            );
        case MAX98357A_SOUND_HORSE:
            return max98357a_write_pcm_u8(
                horse_pcm_start,
                (size_t)(horse_pcm_end - horse_pcm_start)
            );
        case MAX98357A_SOUND_SEALION:
            return max98357a_write_pcm_u8(
                sealion_pcm_start,
                (size_t)(sealion_pcm_end - sealion_pcm_start)
            );
        case MAX98357A_SOUND_MESSAGE:
            return max98357a_write_pcm_u8(
                message_pcm_start,
                (size_t)(message_pcm_end - message_pcm_start)
            );
        default:
            return ESP_ERR_INVALID_ARG;
    }
}

esp_err_t max98357a_play_dog(void)
{
    return max98357a_play_embedded_sound(MAX98357A_SOUND_DOG);
}

esp_err_t max98357a_play_cat(void)
{
    return max98357a_play_embedded_sound(MAX98357A_SOUND_CAT);
}

esp_err_t max98357a_play_cow(void)
{
    return max98357a_play_embedded_sound(MAX98357A_SOUND_COW);
}

esp_err_t max98357a_play_pig(void)
{
    return max98357a_play_embedded_sound(MAX98357A_SOUND_PIG);
}

esp_err_t max98357a_play_horse(void)
{
    return max98357a_play_embedded_sound(MAX98357A_SOUND_HORSE);
}

esp_err_t max98357a_play_sealion(void)
{
    return max98357a_play_embedded_sound(MAX98357A_SOUND_SEALION);
}

esp_err_t max98357a_play_message(void)
{
    return max98357a_play_embedded_sound(MAX98357A_SOUND_MESSAGE);
}

esp_err_t max98357a_play_alarm(void)
{
    if (!s_max98357a_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    const uint32_t beep_count = 6;
    const uint32_t beep_duration_ms = 140;
    const uint32_t pause_duration_ms = 80;
    const uint32_t frequency_hz = 1800;
    const size_t samples_per_beep =
        (MAX98357A_SAMPLE_RATE_HZ * beep_duration_ms) / 1000U;
    const size_t samples_per_pause =
        (MAX98357A_SAMPLE_RATE_HZ * pause_duration_ms) / 1000U;

    uint16_t *buffer = calloc(MAX98357A_BUFFER_SAMPLES, sizeof(uint16_t));
    if (buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const float two_pi = 6.28318530718f;
    for (uint32_t beep = 0; beep < beep_count; ++beep) {
        size_t generated = 0;
        while (generated < samples_per_beep) {
            size_t chunk = MAX98357A_BUFFER_SAMPLES;
            if ((samples_per_beep - generated) < chunk) {
                chunk = samples_per_beep - generated;
            }

            uint8_t live_volume_percent = s_alarm_force_max_volume ? 100U : s_volume_percent;
            if (!s_alarm_force_max_volume &&
                potentiometer_read_percent(&live_volume_percent) == ESP_OK) {
                s_volume_percent = live_volume_percent;
            }

            for (size_t i = 0; i < chunk; ++i) {
                float phase = two_pi * (float)frequency_hz *
                    (float)(generated + i) / (float)MAX98357A_SAMPLE_RATE_HZ;
                int16_t sample = (int16_t)(sinf(phase) * 16000.0f);
                sample = (int16_t)((sample * live_volume_percent) / 100);
                buffer[i] = (uint16_t)sample;
            }

            size_t bytes_written = 0;
            esp_err_t err = i2s_channel_write(
                s_tx_chan,
                buffer,
                chunk * sizeof(uint16_t),
                &bytes_written,
                1000
            );
            if (err != ESP_OK) {
                free(buffer);
                return err;
            }

            generated += chunk;
        }

        for (size_t i = 0; i < MAX98357A_BUFFER_SAMPLES; ++i) {
            buffer[i] = 0;
        }

        generated = 0;
        while (generated < samples_per_pause) {
            size_t chunk = MAX98357A_BUFFER_SAMPLES;
            if ((samples_per_pause - generated) < chunk) {
                chunk = samples_per_pause - generated;
            }

            size_t bytes_written = 0;
            esp_err_t err = i2s_channel_write(
                s_tx_chan,
                buffer,
                chunk * sizeof(uint16_t),
                &bytes_written,
                1000
            );
            if (err != ESP_OK) {
                free(buffer);
                return err;
            }

            generated += chunk;
        }
    }

    free(buffer);
    return ESP_OK;
}

const char *max98357a_sound_to_string(max98357a_sound_t sound)
{
    switch (sound) {
        case MAX98357A_SOUND_DOG:
            return "dog";
        case MAX98357A_SOUND_CAT:
            return "cat";
        case MAX98357A_SOUND_COW:
            return "cow";
        case MAX98357A_SOUND_PIG:
            return "pig";
        case MAX98357A_SOUND_HORSE:
            return "horse";
        case MAX98357A_SOUND_SEALION:
            return "sealion";
        case MAX98357A_SOUND_MESSAGE:
            return "message";
        default:
            return "unknown";
    }
}
