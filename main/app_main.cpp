/**
 * @file main.cpp
 * @brief Integrated ESP32-CAM and Kid Board Messenger App
 */

//--------------------------------- INCLUDES ----------------------------------
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <string>

#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#if CONFIG_WHATSDOWN_CAMERA_NODE
#include "CameraStreamer.h"
#else
#include "FrameReceiver.h"
#include "MQTTManager.h"
#include "MessengerApp.h"
#include "esp_sntp.h"
#include "freertos/queue.h"
#include "gui.h"
#include "max98357a.h"
#include "sensors.h"
#include "ui_app/ui_message_bridge.h"
#include "ui_app/squareline/project/screens/ui_Sketchpad_Scr.h"
#endif

//-------------------------------- DATA TYPES ---------------------------------
#if !CONFIG_WHATSDOWN_CAMERA_NODE
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
#endif

//------------------------- STATIC DATA & CONSTANTS ---------------------------
#if CONFIG_WHATSDOWN_CAMERA_NODE
static CameraStreamer g_streamer(CONFIG_WHATSDOWN_SERVER_BASE_URL "/upload");
#else

class KidBoardReceiver : public FrameReceiver {
 public:
  explicit KidBoardReceiver(const std::string &url) : FrameReceiver(url) {}

 protected:
  void on_frame(const uint8_t *buf, size_t len) override {
    ui_push_camera_frame(buf, len);
  }
};
static KidBoardReceiver g_camera_receiver(CONFIG_WHATSDOWN_SERVER_BASE_URL "/latest");

static MessengerApp *g_messenger_app = nullptr;
static QueueHandle_t g_audio_queue = nullptr;
static volatile bool g_audio_busy = false;
static bool g_apds_ready = false;
static bool g_sketchpad_receiver_paused = false;
static constexpr const char *kPredictUrl =
    CONFIG_WHATSDOWN_SERVER_BASE_URL "/predict";
static constexpr int kPredictSketchW = 96;
static constexpr int kPredictSketchH = 96;
static uint8_t s_predict_gray_buf[kPredictSketchW * kPredictSketchH];
#endif

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
#if !CONFIG_WHATSDOWN_CAMERA_NODE
extern "C" void ui_send_message_from_ui(const char *parent_id, const char *message);
extern "C" void ui_request_message_sound_from_bridge(void);
extern "C" void ui_request_alarm_sound_from_bridge(void);
extern "C" void ui_request_animal_sound_from_bridge(ui_animal_sound_t animal);
extern "C" int ui_request_sketchpad_predict_from_bridge(int *digit_out,
                                                        int *confidence_out);
static void init_time_sync();
static void audio_task(void *parameter);
static void queue_audio_event(audio_event_t event);
static uint32_t sketchpad_sensor_color_name_to_hex(const char *color_name);
static bool parse_predict_response(const char *response, int *digit_out,
                                   int *confidence_out);
typedef struct {
  char *buf;
  size_t cap;
  size_t len;
} predict_http_response_t;
static esp_err_t predict_http_event_handler(esp_http_client_event_t *evt);
#endif

//------------------------------ SHARED FUNCTIONS -----------------------------

static void wifi_init() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config = {};
  snprintf((char *)wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid), "%s",
           CONFIG_WHATSDOWN_WIFI_SSID);
  snprintf((char *)wifi_config.sta.password, sizeof(wifi_config.sta.password),
           "%s", CONFIG_WHATSDOWN_WIFI_PASSWORD);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());

  esp_log_level_set("HTTP_CLIENT", ESP_LOG_NONE);
  esp_log_level_set("transport_base", ESP_LOG_NONE);
  esp_log_level_set("esp-tls", ESP_LOG_NONE);
}

//------------------------ KID BOARD HELPER FUNCTIONS -------------------------
#if !CONFIG_WHATSDOWN_CAMERA_NODE
static void init_time_sync() {
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
  tzset();
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "pool.ntp.org");
  esp_sntp_init();
}

static void queue_audio_event(audio_event_t event) {
  if (g_audio_queue == nullptr) return;
  const bool is_animal_event = (event >= AUDIO_EVENT_DOG && event <= AUDIO_EVENT_SEAL);
  if (is_animal_event && g_audio_busy) return;
  if (xQueueSend(g_audio_queue, &event, 0) != pdTRUE) {
    ESP_LOGW("APP", "Audio queue full, dropping event %d", (int)event);
  }
}

static uint32_t sketchpad_sensor_color_name_to_hex(const char *color_name) {
  if (color_name == nullptr) return 0x1D3557;
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

static bool parse_predict_response(const char *response, int *digit_out,
                                   int *confidence_out) {
  int digit = -1;
  int confidence = 0;
  float confidence_f = 0.0f;

  if (!response || !digit_out || !confidence_out) return false;

  if (sscanf(response, "{\"digit\":%d,\"confidence\":%d}", &digit, &confidence) ==
      2) {
    *digit_out = digit;
    *confidence_out = confidence;
    return true;
  }

  if (sscanf(response,
             "{\"digit\":%d,\"confidence\":%f}",
             &digit, &confidence_f) == 2) {
    *digit_out = digit;
    *confidence_out = (int)(confidence_f <= 1.0f ? confidence_f * 100.0f
                                                 : confidence_f);
    return true;
  }

  if (sscanf(response, "{\"class\":%d,\"confidence\":%d}", &digit, &confidence) ==
      2) {
    *digit_out = digit;
    *confidence_out = confidence;
    return true;
  }

  if (sscanf(response, "{\"prediction\":%d}", &digit) == 1) {
    *digit_out = digit;
    *confidence_out = 0;
    return true;
  }

  for (const char *p = response; *p; ++p) {
    if (*p >= '0' && *p <= '9') {
      *digit_out = *p - '0';
      *confidence_out = 0;
      return true;
    }
  }

  return false;
}

static esp_err_t predict_http_event_handler(esp_http_client_event_t *evt) {
  if (!evt || !evt->user_data) return ESP_OK;

  predict_http_response_t *resp =
      static_cast<predict_http_response_t *>(evt->user_data);

  if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data && evt->data_len > 0 &&
      resp->buf && resp->cap > 0) {
    size_t copy_len = (size_t)evt->data_len;
    size_t remaining = (resp->cap - 1 > resp->len) ? (resp->cap - 1 - resp->len) : 0;
    if (copy_len > remaining) copy_len = remaining;
    if (copy_len > 0) {
      memcpy(resp->buf + resp->len, evt->data, copy_len);
      resp->len += copy_len;
      resp->buf[resp->len] = '\0';
    }
  }

  return ESP_OK;
}

static void audio_task(void *parameter) {
  (void)parameter;
  bool audio_ready = false;
  audio_event_t event = AUDIO_EVENT_MESSAGE;
  for (;;) {
    if (xQueueReceive(g_audio_queue, &event, portMAX_DELAY) != pdTRUE) continue;
    g_audio_busy = true;
    if (!audio_ready) {
      if (max98357a_init() != ESP_OK) {
        g_audio_busy = false;
        continue;
      }
      audio_ready = true;
    }
    const int repeat_count =
        (event >= AUDIO_EVENT_DOG && event <= AUDIO_EVENT_SEAL) ? 3 : 1;
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
            if (max98357a_play_alarm() != ESP_OK) break;
          }
          max98357a_set_alarm_force_max_volume(false);
          break;
        default:
          err = ESP_ERR_INVALID_ARG;
          break;
      }
      if (err != ESP_OK) break;
      if (attempt < (repeat_count - 1)) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    g_audio_busy = false;
  }
}

extern "C" void ui_send_message_from_ui(const char *parent_id, const char *message) {
  if (g_messenger_app) g_messenger_app->send_to_parent(parent_id, message);
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

extern "C" int ui_request_sketchpad_sensor_color_from_bridge(
    uint32_t *color_hex_out, char *color_name_out, size_t color_name_out_size) {
  apds9960_color_data_t color = {};
  if (!color_hex_out || !g_apds_ready) return 0;
  if (sensors_read_apds9960_color(&color) != ESP_OK) return 0;
  const char *detected_name = sensors_detect_apds9960_color_name(&color);
  *color_hex_out = sketchpad_sensor_color_name_to_hex(detected_name);
  if (color_name_out)
    snprintf(color_name_out, color_name_out_size, "%s",
             detected_name ? detected_name : "UNKNOWN");
  return 1;
}

extern "C" void ui_notify_sketchpad_enter_from_bridge(void) {
  if (!g_sketchpad_receiver_paused) {
    ESP_LOGI("APP", "Entering sketchpad mode: pausing frame receiver");
    g_camera_receiver.stop();
    g_sketchpad_receiver_paused = true;
  }
}

extern "C" void ui_notify_sketchpad_exit_from_bridge(void) {
  if (g_sketchpad_receiver_paused) {
    ESP_LOGI("APP", "Leaving sketchpad mode: resuming frame receiver");
    g_camera_receiver.start();
    g_sketchpad_receiver_paused = false;
  }
}

extern "C" int ui_request_sketchpad_predict_from_bridge(int *digit_out,
                                                        int *confidence_out) {
  static constexpr size_t kGraySize = kPredictSketchW * kPredictSketchH;
  static constexpr int kPredictAttempts = 3;
  static constexpr TickType_t kPredictRetryDelay = pdMS_TO_TICKS(1500);

  char response[128] = {0};
  char width_header[8] = {0};
  char height_header[8] = {0};
  int digit = -1;
  int confidence = 0;
  int result = 0;
  bool paused_for_predict = false;
  int attempt = 0;

  if (!digit_out || !confidence_out) return 0;

  if (!g_sketchpad_receiver_paused) {
    ESP_LOGI("APP", "Predict: temporarily pausing frame receiver");
    g_camera_receiver.stop();
    g_sketchpad_receiver_paused = true;
    paused_for_predict = true;
    vTaskDelay(pdMS_TO_TICKS(100));
  }

  if (!ui_Sketchpad_render_gray8(s_predict_gray_buf, kGraySize, kPredictSketchW,
                                 kPredictSketchH)) {
    ESP_LOGE("APP", "Sketchpad render failed");
    goto cleanup;
  }

  snprintf(width_header, sizeof(width_header), "%d", kPredictSketchW);
  snprintf(height_header, sizeof(height_header), "%d", kPredictSketchH);
  for (attempt = 1; attempt <= kPredictAttempts; ++attempt) {
    esp_http_client_config_t config = {};
    predict_http_response_t http_response = {
        .buf = response,
        .cap = sizeof(response),
        .len = 0,
    };
    esp_http_client_handle_t client = nullptr;

    memset(response, 0, sizeof(response));

    config.url = kPredictUrl;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 8000;
    config.keep_alive_enable = false;
    config.event_handler = predict_http_event_handler;
    config.user_data = &http_response;
    client = esp_http_client_init(&config);
    if (!client) {
      ESP_LOGW("APP", "Predict init failed (attempt %d/%d)", attempt,
               kPredictAttempts);
    } else {
      esp_http_client_set_header(client, "Content-Type",
                                 "application/octet-stream");
      esp_http_client_set_header(client, "X-Width", width_header);
      esp_http_client_set_header(client, "X-Height", height_header);
      esp_http_client_set_header(client, "X-Format", "gray8");
      esp_http_client_set_post_field(
          client, reinterpret_cast<const char *>(s_predict_gray_buf),
          (int)kGraySize);

      {
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK && http_response.len > 0 &&
            parse_predict_response(response, &digit, &confidence)) {
          *digit_out = digit;
          *confidence_out = confidence;
          ESP_LOGI("APP", "Predict result: %d (%d%%)", digit, confidence);
          result = 1;
        } else if (err != ESP_OK) {
          ESP_LOGW("APP", "Predict POST failed (attempt %d/%d): %s", attempt,
                   kPredictAttempts, esp_err_to_name(err));
        } else if (http_response.len == 0) {
          ESP_LOGW("APP", "Predict response empty (attempt %d/%d)", attempt,
                   kPredictAttempts);
        } else {
          ESP_LOGW("APP", "Predict response invalid (attempt %d/%d): %s",
                   attempt, kPredictAttempts, response);
        }
      }
    }

    if (client) {
      esp_http_client_cleanup(client);
    }

    if (result == 1) {
      break;
    }

    if (attempt < kPredictAttempts) {
      vTaskDelay(kPredictRetryDelay);
    }
  }

cleanup:
  if (paused_for_predict) {
    ESP_LOGI("APP", "Predict: resuming frame receiver");
    g_camera_receiver.start();
    g_sketchpad_receiver_paused = false;
  }
  return result;
}
#endif

//------------------------------ PUBLIC FUNCTIONS -----------------------------

extern "C" void app_main(void) {
  // Shared WiFi Initialization
  wifi_init();

#if CONFIG_WHATSDOWN_CAMERA_NODE
  // ---------------------------------------------------------
  // CAMERA MODE LOGIC
  // ---------------------------------------------------------
  ESP_LOGI("APP", "Starting in CAMERA NODE mode");

  vTaskDelay(pdMS_TO_TICKS(3000));

  if (g_streamer.init() == ESP_OK) {
    g_streamer.start();
    ESP_LOGI("APP", "Streamer task running at 20 FPS (QVGA)");
  } else {
    ESP_LOGE("APP", "Camera hardware failure");
  }

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

#else
  // ---------------------------------------------------------
  // KID BOARD MODE LOGIC
  // ---------------------------------------------------------
  ESP_LOGI("APP", "Starting in KID BOARD mode");

  if (sensors_init_apds9960() == ESP_OK) {
    g_apds_ready = true;
  }

  ui_register_sketchpad_sensor_color_callback(
      ui_request_sketchpad_sensor_color_from_bridge);
  ui_register_sketchpad_predict_callback(
      ui_request_sketchpad_predict_from_bridge);
  gui_init();

  g_audio_queue = xQueueCreate(8, sizeof(audio_event_t));
  if (g_audio_queue != nullptr) {
    xTaskCreatePinnedToCore(audio_task, "audio_task", 4096, nullptr, 2, nullptr, 0);
  }

  vTaskDelay(pdMS_TO_TICKS(5000));  // Time for WiFi to connect
  init_time_sync();

  g_camera_receiver.start();  // Start pulling frames from Python server

  MQTTManager &mqtt = MQTTManager::get_instance();
  mqtt.connect(CONFIG_WHATSDOWN_MQTT_URI);

  static MessengerApp app(mqtt, CONFIG_WHATSDOWN_KID_ID);
  app.begin();
  g_messenger_app = &app;

  ui_register_send_message_callback(ui_send_message_from_ui);
  ui_register_message_sound_callback(ui_request_message_sound_from_bridge);
  ui_register_alarm_sound_callback(ui_request_alarm_sound_from_bridge);
  ui_register_play_animal_sound_callback(ui_request_animal_sound_from_bridge);

  int last_alarm_yday = -1, last_alarm_hour = -1, last_alarm_minute = -1;

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
          ui_request_alarm_sound_from_bridge();
          last_alarm_yday = timeinfo.tm_yday;
          last_alarm_hour = timeinfo.tm_hour;
          last_alarm_minute = timeinfo.tm_min;
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
#endif
}
