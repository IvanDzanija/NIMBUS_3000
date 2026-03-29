/**
* @file main.c

* @brief
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

//--------------------------------- INCLUDES ----------------------------------
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>

#include "MQTTManager.h"
#include "MessangerApp.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "gui.h"
#include "max98357a.h"
#include "nvs_flash.h"
#include "sensors.h"
#include "ui_app/ui_message_bridge.h"
//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

static MessengerApp *g_messenger_app = nullptr;
static QueueHandle_t g_audio_queue = nullptr;
static volatile bool g_audio_busy = false;
static bool g_apds_ready = false;

typedef enum {
  AUDIO_EVENT_MESSAGE = 1,
  AUDIO_EVENT_DOG,
  AUDIO_EVENT_CAT,
  AUDIO_EVENT_COW,
  AUDIO_EVENT_PIG,
  AUDIO_EVENT_HORSE,
  AUDIO_EVENT_SEAL,
  AUDIO_EVENT_ALARM,
} audio_event_t;

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
extern "C" void ui_send_message_from_ui(const char *parent_id, const char *message);
extern "C" void ui_request_message_sound_from_bridge(void);
extern "C" void ui_request_alarm_sound_from_bridge(void);
extern "C" void ui_request_animal_sound_from_bridge(ui_animal_sound_t animal);
static void init_time_sync();
static void audio_task(void *parameter);
static void queue_audio_event(audio_event_t event);
static uint32_t sketchpad_sensor_color_name_to_hex(const char *color_name);

//------------------------- STATIC DATA & CONSTANTS ---------------------------

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------
void wifi_init() {
  nvs_flash_init();
  esp_netif_init();
  esp_event_loop_create_default();

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = "Bird",
              .password = "BIRD*2025*",
          },
  };

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();

  esp_wifi_connect();
}

static void init_time_sync() {
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_init();
}

static void queue_audio_event(audio_event_t event) {
  if (g_audio_queue == nullptr) {
    return;
  }

  const bool is_animal_event =
      (event == AUDIO_EVENT_DOG || event == AUDIO_EVENT_CAT ||
       event == AUDIO_EVENT_COW || event == AUDIO_EVENT_PIG ||
       event == AUDIO_EVENT_HORSE || event == AUDIO_EVENT_SEAL);

  if (is_animal_event && g_audio_busy) {
    return;
  }

  if (xQueueSend(g_audio_queue, &event, 0) != pdTRUE) {
    ESP_LOGW("APP", "Audio queue full, dropping event %d", (int)event);
  }
}

static uint32_t sketchpad_sensor_color_name_to_hex(const char *color_name) {
  if (color_name == nullptr) {
    return 0x1D3557;
  }

  if (strcmp(color_name, "RED") == 0) return 0xE63946;
  if (strcmp(color_name, "GREEN") == 0) return 0x2A9D8F;
  if (strcmp(color_name, "BLUE") == 0) return 0x457B9D;
  if (strcmp(color_name, "YELLOW") == 0) return 0xE9C46A;
  if (strcmp(color_name, "ORANGE") == 0) return 0xF4A261;
  if (strcmp(color_name, "MAGENTA") == 0) return 0xC855BC;
  if (strcmp(color_name, "CYAN") == 0) return 0x4CC9F0;
  if (strcmp(color_name, "WHITE") == 0) return 0xF1F1F1;
  if (strcmp(color_name, "DARK") == 0) return 0x202020;

  return 0x1D3557;
}

static void audio_task(void *parameter) {
  (void)parameter;

  bool audio_ready = false;
  audio_event_t event = AUDIO_EVENT_MESSAGE;

  for (;;) {
    if (xQueueReceive(g_audio_queue, &event, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    g_audio_busy = true;

    if (!audio_ready) {
      esp_err_t init_err = max98357a_init();
      if (init_err != ESP_OK) {
        ESP_LOGW("APP", "MAX98357A init failed: %s", esp_err_to_name(init_err));
        g_audio_busy = false;
        continue;
      }
      audio_ready = true;
    }

    const int repeat_count =
        (event == AUDIO_EVENT_DOG || event == AUDIO_EVENT_CAT ||
         event == AUDIO_EVENT_COW || event == AUDIO_EVENT_PIG ||
         event == AUDIO_EVENT_HORSE || event == AUDIO_EVENT_SEAL)
            ? 3
            : 1;

    for (int attempt = 0; attempt < repeat_count; ++attempt) {
      esp_err_t err = ESP_OK;
      switch (event) {
        case AUDIO_EVENT_MESSAGE:
          err = max98357a_play_message();
          break;
        case AUDIO_EVENT_DOG:
          err = max98357a_play_dog();
          break;
        case AUDIO_EVENT_CAT:
          err = max98357a_play_cat();
          break;
        case AUDIO_EVENT_COW:
          err = max98357a_play_cow();
          break;
        case AUDIO_EVENT_PIG:
          err = max98357a_play_pig();
          break;
        case AUDIO_EVENT_HORSE:
          err = max98357a_play_horse();
          break;
      case AUDIO_EVENT_SEAL:
        err = max98357a_play_sealion();
        break;
      case AUDIO_EVENT_ALARM:
        max98357a_set_alarm_force_max_volume(true);
        while (ui_is_alarm_ringing()) {
          err = max98357a_play_alarm();
          if (err != ESP_OK) {
            break;
          }
        }
        max98357a_set_alarm_force_max_volume(false);
        break;
      default:
        err = ESP_ERR_INVALID_ARG;
        break;
      }

      if (err != ESP_OK) {
        ESP_LOGW("APP", "Audio playback failed for event %d: %s", (int)event,
                 esp_err_to_name(err));
        break;
      }

      if (attempt < (repeat_count - 1)) {
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
    }

    g_audio_busy = false;
  }
}

extern "C" void ui_send_message_from_ui(const char *parent_id, const char *message) {
  if (g_messenger_app == nullptr || parent_id == nullptr || message == nullptr) {
    return;
  }

  g_messenger_app->send_to_parent(parent_id, message);
}

extern "C" void ui_request_message_sound_from_bridge(void) {
  queue_audio_event(AUDIO_EVENT_MESSAGE);
}

extern "C" void ui_request_alarm_sound_from_bridge(void) {
  queue_audio_event(AUDIO_EVENT_ALARM);
}

extern "C" void ui_request_animal_sound_from_bridge(ui_animal_sound_t animal) {
  switch (animal) {
    case UI_ANIMAL_SOUND_DOG:
      queue_audio_event(AUDIO_EVENT_DOG);
      break;
    case UI_ANIMAL_SOUND_CAT:
      queue_audio_event(AUDIO_EVENT_CAT);
      break;
    case UI_ANIMAL_SOUND_COW:
      queue_audio_event(AUDIO_EVENT_COW);
      break;
    case UI_ANIMAL_SOUND_PIG:
      queue_audio_event(AUDIO_EVENT_PIG);
      break;
    case UI_ANIMAL_SOUND_HORSE:
      queue_audio_event(AUDIO_EVENT_HORSE);
      break;
    case UI_ANIMAL_SOUND_SEAL:
      queue_audio_event(AUDIO_EVENT_SEAL);
      break;
    default:
      break;
  }
}

extern "C" int ui_request_sketchpad_sensor_color_from_bridge(uint32_t *color_hex_out,
                                                             char *color_name_out,
                                                             size_t color_name_out_size) {
  apds9960_color_data_t color = {};
  const char *detected_name = nullptr;

  if (color_hex_out == nullptr) {
    return 0;
  }

  if (!g_apds_ready) {
    ESP_LOGW("APP", "APDS-9960 is not ready");
    return 0;
  }

  if (sensors_read_apds9960_color(&color) != ESP_OK) {
    ESP_LOGW("APP", "Failed to read APDS-9960 color");
    return 0;
  }

  detected_name = sensors_detect_apds9960_color_name(&color);
  *color_hex_out = sketchpad_sensor_color_name_to_hex(detected_name);

  if (color_name_out != nullptr && color_name_out_size > 0) {
    snprintf(color_name_out, color_name_out_size, "%s", detected_name ? detected_name : "UNKNOWN");
  }

  ESP_LOGI("APP", "Sketchpad color selected via APDS-9960: %s (c=%u r=%u g=%u b=%u)",
           detected_name ? detected_name : "UNKNOWN",
           (unsigned)color.clear,
           (unsigned)color.red,
           (unsigned)color.green,
           (unsigned)color.blue);
  return 1;
}

extern "C" void app_main(void) {
  if (sensors_init_apds9960() == ESP_OK) {
    g_apds_ready = true;
  } else {
    ESP_LOGW("APP", "APDS-9960 init failed, Sketchpad color sensor will be unavailable");
  }

  ui_register_sketchpad_sensor_color_callback(ui_request_sketchpad_sensor_color_from_bridge);

  gui_init();
  wifi_init();

  g_audio_queue = xQueueCreate(8, sizeof(audio_event_t));
  if (g_audio_queue != nullptr) {
    xTaskCreatePinnedToCore(audio_task, "audio_task", 4096, nullptr, 2, nullptr,
                            0);
  } else {
    ESP_LOGW("APP", "Failed to create audio queue");
  }

  // Give WiFi a moment to connect
  vTaskDelay(pdMS_TO_TICKS(5000));
  init_time_sync();

  MQTTManager &mqtt = MQTTManager::get_instance();
  mqtt.connect("mqtt://172.16.55.93:1883");

  MessengerApp app(mqtt, "kid");
  app.begin();
  g_messenger_app = &app;
  ui_register_send_message_callback(ui_send_message_from_ui);
  ui_register_message_sound_callback(ui_request_message_sound_from_bridge);
  ui_register_alarm_sound_callback(ui_request_alarm_sound_from_bridge);
  ui_register_play_animal_sound_callback(ui_request_animal_sound_from_bridge);

  int last_alarm_yday = -1;
  int last_alarm_hour = -1;
  int last_alarm_minute = -1;
  while (1) {
    if (ui_is_alarm_enabled()) {
      time_t now;
      struct tm timeinfo;
      time(&now);
      localtime_r(&now, &timeinfo);

      int alarm_hour = ui_get_alarm_hour();
      int alarm_minute = ui_get_alarm_minute();
      if (timeinfo.tm_hour == alarm_hour && timeinfo.tm_min == alarm_minute) {
        if (timeinfo.tm_yday != last_alarm_yday ||
            timeinfo.tm_hour != last_alarm_hour ||
            timeinfo.tm_min != last_alarm_minute) {
          ui_set_alarm_ringing(1);
          ui_request_alarm_sound();
          last_alarm_yday = timeinfo.tm_yday;
          last_alarm_hour = timeinfo.tm_hour;
          last_alarm_minute = timeinfo.tm_min;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
//---------------------------- PRIVATE FUNCTIONS ------------------------------

//---------------------------- INTERRUPT HANDLERS -----------------------------
