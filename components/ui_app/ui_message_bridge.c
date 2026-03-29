/**
 * @file ui_message_bridge.c
 */

#include "ui_message_bridge.h"
#include <stdio.h>

#include "freertos/FreeRTOS.h"

#define UI_MESSAGE_HISTORY_COUNT 10
#define UI_MESSAGE_SENDER_LEN 16
#define UI_MESSAGE_TEXT_LEN 128

typedef struct {
    char sender[UI_MESSAGE_SENDER_LEN];
    char text[UI_MESSAGE_TEXT_LEN];
} ui_received_message_t;

static ui_send_message_fn_t s_send_message_callback = 0;
static ui_message_sound_fn_t s_message_sound_callback = 0;
static ui_alarm_sound_fn_t s_alarm_sound_callback = 0;
static ui_play_animal_sound_fn_t s_play_animal_sound_callback = 0;
static ui_sketchpad_sensor_color_fn_t s_sketchpad_sensor_color_callback = 0;
static ui_sketchpad_predict_fn_t s_sketchpad_predict_callback = 0;
static ui_received_message_t s_received_messages[UI_MESSAGE_HISTORY_COUNT];
static int s_received_message_count = 0;
static unsigned int s_received_message_version = 0;
static int s_alarm_hour = 7;
static int s_alarm_minute = 0;
static int s_alarm_enabled = 0;
static int s_alarm_ringing = 0;
static unsigned int s_alarm_trigger_version = 0;
static portMUX_TYPE s_message_lock = portMUX_INITIALIZER_UNLOCKED;

void ui_register_send_message_callback(ui_send_message_fn_t callback)
{
    s_send_message_callback = callback;
}

void ui_register_message_sound_callback(ui_message_sound_fn_t callback)
{
    s_message_sound_callback = callback;
}

void ui_register_alarm_sound_callback(ui_alarm_sound_fn_t callback)
{
    s_alarm_sound_callback = callback;
}

void ui_register_play_animal_sound_callback(ui_play_animal_sound_fn_t callback)
{
    s_play_animal_sound_callback = callback;
}

void ui_register_sketchpad_sensor_color_callback(ui_sketchpad_sensor_color_fn_t callback)
{
    s_sketchpad_sensor_color_callback = callback;
}

void ui_register_sketchpad_predict_callback(ui_sketchpad_predict_fn_t callback)
{
    s_sketchpad_predict_callback = callback;
}

int ui_send_message_to_parent(const char *parent_id, const char *message)
{
    if (s_send_message_callback == 0 || parent_id == 0 || message == 0) {
        return 0;
    }

    s_send_message_callback(parent_id, message);
    return 1;
}

int ui_request_message_sound(void)
{
    if (s_message_sound_callback == 0) {
        return 0;
    }

    s_message_sound_callback();
    return 1;
}

int ui_request_alarm_sound(void)
{
    if (s_alarm_sound_callback == 0) {
        return 0;
    }

    s_alarm_sound_callback();
    return 1;
}

int ui_request_animal_sound(ui_animal_sound_t animal)
{
    if (s_play_animal_sound_callback == 0) {
        return 0;
    }

    s_play_animal_sound_callback(animal);
    return 1;
}

int ui_request_sketchpad_sensor_color(uint32_t *color_hex_out, char *color_name_out, size_t color_name_out_size)
{
    if (s_sketchpad_sensor_color_callback == 0 || color_hex_out == 0) {
        return 0;
    }

    return s_sketchpad_sensor_color_callback(color_hex_out, color_name_out, color_name_out_size);
}

int ui_request_sketchpad_predict(int *digit_out, int *confidence_out)
{
    if (s_sketchpad_predict_callback == 0 || digit_out == 0 || confidence_out == 0) {
        return 0;
    }

    return s_sketchpad_predict_callback(digit_out, confidence_out);
}

void ui_notify_sketchpad_enter(void)
{
    extern void ui_notify_sketchpad_enter_from_bridge(void);
    ui_notify_sketchpad_enter_from_bridge();
}

void ui_notify_sketchpad_exit(void)
{
    extern void ui_notify_sketchpad_exit_from_bridge(void);
    ui_notify_sketchpad_exit_from_bridge();
}

void ui_set_alarm_time(int hour, int minute)
{
    if (hour < 0) hour = 0;
    if (hour > 23) hour = 23;
    if (minute < 0) minute = 0;
    if (minute > 59) minute = 59;

    taskENTER_CRITICAL(&s_message_lock);
    s_alarm_hour = hour;
    s_alarm_minute = minute;
    taskEXIT_CRITICAL(&s_message_lock);
}

int ui_get_alarm_hour(void)
{
    int hour;

    taskENTER_CRITICAL(&s_message_lock);
    hour = s_alarm_hour;
    taskEXIT_CRITICAL(&s_message_lock);

    return hour;
}

int ui_get_alarm_minute(void)
{
    int minute;

    taskENTER_CRITICAL(&s_message_lock);
    minute = s_alarm_minute;
    taskEXIT_CRITICAL(&s_message_lock);

    return minute;
}

void ui_set_alarm_enabled(int enabled)
{
    taskENTER_CRITICAL(&s_message_lock);
    s_alarm_enabled = enabled ? 1 : 0;
    taskEXIT_CRITICAL(&s_message_lock);
}

int ui_is_alarm_enabled(void)
{
    int enabled;

    taskENTER_CRITICAL(&s_message_lock);
    enabled = s_alarm_enabled;
    taskEXIT_CRITICAL(&s_message_lock);

    return enabled;
}

void ui_set_alarm_ringing(int ringing)
{
    taskENTER_CRITICAL(&s_message_lock);
    s_alarm_ringing = ringing ? 1 : 0;
    if (s_alarm_ringing) {
        s_alarm_trigger_version++;
    }
    taskEXIT_CRITICAL(&s_message_lock);
}

int ui_is_alarm_ringing(void)
{
    int ringing;

    taskENTER_CRITICAL(&s_message_lock);
    ringing = s_alarm_ringing;
    taskEXIT_CRITICAL(&s_message_lock);

    return ringing;
}

unsigned int ui_get_alarm_trigger_version(void)
{
    unsigned int version;

    taskENTER_CRITICAL(&s_message_lock);
    version = s_alarm_trigger_version;
    taskEXIT_CRITICAL(&s_message_lock);

    return version;
}

void ui_store_received_message(const char *sender_id, const char *message)
{
    int index;

    if (sender_id == 0 || message == 0) {
        return;
    }

    taskENTER_CRITICAL(&s_message_lock);

    if (s_received_message_count < UI_MESSAGE_HISTORY_COUNT) {
        index = s_received_message_count++;
    } else {
        for (index = 1; index < UI_MESSAGE_HISTORY_COUNT; index++) {
            s_received_messages[index - 1] = s_received_messages[index];
        }
        index = UI_MESSAGE_HISTORY_COUNT - 1;
    }

    snprintf(s_received_messages[index].sender, sizeof(s_received_messages[index].sender), "%s", sender_id);
    snprintf(s_received_messages[index].text, sizeof(s_received_messages[index].text), "%s", message);
    s_received_message_version++;

    taskEXIT_CRITICAL(&s_message_lock);
}

int ui_get_received_message_count(void)
{
    int count;

    taskENTER_CRITICAL(&s_message_lock);
    count = s_received_message_count;
    taskEXIT_CRITICAL(&s_message_lock);

    return count;
}

const char *ui_get_received_message_sender(int index)
{
    const char *sender = "";

    taskENTER_CRITICAL(&s_message_lock);
    if (index >= 0 && index < s_received_message_count) {
        sender = s_received_messages[index].sender;
    }
    taskEXIT_CRITICAL(&s_message_lock);

    return sender;
}

const char *ui_get_received_message_text(int index)
{
    const char *text = "";

    taskENTER_CRITICAL(&s_message_lock);
    if (index >= 0 && index < s_received_message_count) {
        text = s_received_messages[index].text;
    }
    taskEXIT_CRITICAL(&s_message_lock);

    return text;
}

unsigned int ui_get_received_message_version(void)
{
    unsigned int version;

    taskENTER_CRITICAL(&s_message_lock);
    version = s_received_message_version;
    taskEXIT_CRITICAL(&s_message_lock);

    return version;
}
