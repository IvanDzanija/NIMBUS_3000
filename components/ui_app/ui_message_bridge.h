/**
 * @file ui_message_bridge.h
 */

#ifndef UI_MESSAGE_BRIDGE_H
#define UI_MESSAGE_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ui_send_message_fn_t)(const char *parent_id, const char *message);
typedef void (*ui_message_sound_fn_t)(void);
typedef void (*ui_alarm_sound_fn_t)(void);
typedef int (*ui_sketchpad_sensor_color_fn_t)(uint32_t *color_hex_out, char *color_name_out, size_t color_name_out_size);

typedef enum {
    UI_ANIMAL_SOUND_DOG = 0,
    UI_ANIMAL_SOUND_CAT,
    UI_ANIMAL_SOUND_COW,
    UI_ANIMAL_SOUND_PIG,
    UI_ANIMAL_SOUND_HORSE,
    UI_ANIMAL_SOUND_SEAL,
} ui_animal_sound_t;

typedef void (*ui_play_animal_sound_fn_t)(ui_animal_sound_t animal);

void ui_register_send_message_callback(ui_send_message_fn_t callback);
void ui_register_message_sound_callback(ui_message_sound_fn_t callback);
void ui_register_alarm_sound_callback(ui_alarm_sound_fn_t callback);
void ui_register_play_animal_sound_callback(ui_play_animal_sound_fn_t callback);
void ui_register_sketchpad_sensor_color_callback(ui_sketchpad_sensor_color_fn_t callback);
int ui_send_message_to_parent(const char *parent_id, const char *message);
int ui_request_message_sound(void);
int ui_request_alarm_sound(void);
int ui_request_animal_sound(ui_animal_sound_t animal);
int ui_request_sketchpad_sensor_color(uint32_t *color_hex_out, char *color_name_out, size_t color_name_out_size);
void ui_set_alarm_time(int hour, int minute);
int ui_get_alarm_hour(void);
int ui_get_alarm_minute(void);
void ui_set_alarm_enabled(int enabled);
int ui_is_alarm_enabled(void);
void ui_set_alarm_ringing(int ringing);
int ui_is_alarm_ringing(void);
unsigned int ui_get_alarm_trigger_version(void);
void ui_store_received_message(const char *sender_id, const char *message);
int ui_get_received_message_count(void);
const char *ui_get_received_message_sender(int index);
const char *ui_get_received_message_text(int index);
unsigned int ui_get_received_message_version(void);

#ifdef __cplusplus
}
#endif

#endif /* UI_MESSAGE_BRIDGE_H */
