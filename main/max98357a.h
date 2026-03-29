#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX98357A_BCLK_GPIO GPIO_NUM_26
#define MAX98357A_LRC_GPIO  GPIO_NUM_27
#define MAX98357A_DIN_GPIO  GPIO_NUM_14
#define MAX98357A_SAMPLE_RATE_HZ 44100
#define MAX98357A_BUFFER_SAMPLES 2048

typedef enum {
    MAX98357A_SOUND_DOG = 0,
    MAX98357A_SOUND_CAT,
    MAX98357A_SOUND_COW,
    MAX98357A_SOUND_PIG,
    MAX98357A_SOUND_HORSE,
    MAX98357A_SOUND_SEALION,
    MAX98357A_SOUND_MESSAGE,
} max98357a_sound_t;

esp_err_t max98357a_init(void);
esp_err_t max98357a_deinit(void);
void max98357a_set_volume_percent(uint8_t percent);
uint8_t max98357a_get_volume_percent(void);
void max98357a_set_alarm_force_max_volume(bool enabled);
esp_err_t max98357a_write_pcm_u8(const uint8_t *pcm_data, size_t pcm_length);
esp_err_t max98357a_play_embedded_sound(max98357a_sound_t sound);
esp_err_t max98357a_play_dog(void);
esp_err_t max98357a_play_cat(void);
esp_err_t max98357a_play_cow(void);
esp_err_t max98357a_play_pig(void);
esp_err_t max98357a_play_horse(void);
esp_err_t max98357a_play_sealion(void);
esp_err_t max98357a_play_message(void);
esp_err_t max98357a_play_alarm(void);
const char *max98357a_sound_to_string(max98357a_sound_t sound);

#ifdef __cplusplus
}
#endif
