/**
 * @file main.cpp
 * @brief Integrated ESP32-CAM and Kid Board Messenger App
 */

// ============================================================================
// TARGET SELECTION: Toggle this macro to switch between boards
// ============================================================================
#define USE_CAMERA_NODE  // Uncomment this when building for ESP32-CAM
// ============================================================================

//--------------------------------- INCLUDES ----------------------------------
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef USE_CAMERA_NODE
    #include "CameraStreamer.h"
#else
    #include "MQTTManager.h"
    #include "MessangerApp.h"
    #include "esp_sntp.h"
    #include "freertos/queue.h"
    #include "gui.h"
    #include "max98357a.h"
    #include "sensors.h"
    #include "ui_app/ui_message_bridge.h"
#endif

//-------------------------------- DATA TYPES ---------------------------------
#ifndef USE_CAMERA_NODE
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
#ifdef USE_CAMERA_NODE
    static CameraStreamer g_streamer("http://172.16.55.93:8080/upload");
#else
    static MessengerApp *g_messenger_app = nullptr;
    static QueueHandle_t g_audio_queue = nullptr;
    static volatile bool g_audio_busy = false;
    static bool g_apds_ready = false;
#endif

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
#ifndef USE_CAMERA_NODE
extern "C" void ui_send_message_from_ui(const char *parent_id, const char *message);
extern "C" void ui_request_message_sound_from_bridge(void);
extern "C" void ui_request_alarm_sound_from_bridge(void);
extern "C" void ui_request_animal_sound_from_bridge(ui_animal_sound_t animal);
static void init_time_sync();
static void audio_task(void *parameter);
static void queue_audio_event(audio_event_t event);
static uint32_t sketchpad_sensor_color_name_to_hex(const char *color_name);
#endif

//------------------------------ SHARED FUNCTIONS -----------------------------

static void wifi_init() {
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, "Bird");
    strcpy((char*)wifi_config.sta.password, "BIRD*2025*");

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
}

//------------------------ KID BOARD HELPER FUNCTIONS -------------------------
#ifndef USE_CAMERA_NODE
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
        const int repeat_count = (event >= AUDIO_EVENT_DOG && event <= AUDIO_EVENT_SEAL) ? 3 : 1;
        for (int attempt = 0; attempt < repeat_count; ++attempt) {
            esp_err_t err = ESP_OK;
            switch (event) {
                case AUDIO_EVENT_MESSAGE: err = max98357a_play_message(); break;
                case AUDIO_EVENT_DOG:     err = max98357a_play_dog(); break;
                case AUDIO_EVENT_CAT:     err = max98357a_play_cat(); break;
                case AUDIO_EVENT_COW:     err = max98357a_play_cow(); break;
                case AUDIO_EVENT_PIG:     err = max98357a_play_pig(); break;
                case AUDIO_EVENT_HORSE:   err = max98357a_play_horse(); break;
                case AUDIO_EVENT_SEAL:    err = max98357a_play_sealion(); break;
                case AUDIO_EVENT_ALARM:
                    max98357a_set_alarm_force_max_volume(true);
                    while (ui_is_alarm_ringing()) {
                        if (max98357a_play_alarm() != ESP_OK) break;
                    }
                    max98357a_set_alarm_force_max_volume(false);
                    break;
                default: err = ESP_ERR_INVALID_ARG; break;
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
extern "C" void ui_request_message_sound_from_bridge(void) { queue_audio_event(AUDIO_EVENT_MESSAGE); }
extern "C" void ui_request_alarm_sound_from_bridge(void) { queue_audio_event(AUDIO_EVENT_ALARM); }
extern "C" void ui_request_animal_sound_from_bridge(ui_animal_sound_t animal) {
    switch (animal) {
        case UI_ANIMAL_SOUND_DOG:   queue_audio_event(AUDIO_EVENT_DOG); break;
        case UI_ANIMAL_SOUND_CAT:   queue_audio_event(AUDIO_EVENT_CAT); break;
        case UI_ANIMAL_SOUND_COW:   queue_audio_event(AUDIO_EVENT_COW); break;
        case UI_ANIMAL_SOUND_PIG:   queue_audio_event(AUDIO_EVENT_PIG); break;
        case UI_ANIMAL_SOUND_HORSE: queue_audio_event(AUDIO_EVENT_HORSE); break;
        case UI_ANIMAL_SOUND_SEAL:  queue_audio_event(AUDIO_EVENT_SEAL); break;
        default: break;
    }
}

extern "C" int ui_request_sketchpad_sensor_color_from_bridge(uint32_t *color_hex_out, char *color_name_out, size_t color_name_out_size) {
    apds9960_color_data_t color = {};
    if (!color_hex_out || !g_apds_ready) return 0;
    if (sensors_read_apds9960_color(&color) != ESP_OK) return 0;
    const char *detected_name = sensors_detect_apds9960_color_name(&color);
    *color_hex_out = sketchpad_sensor_color_name_to_hex(detected_name);
    if (color_name_out) snprintf(color_name_out, color_name_out_size, "%s", detected_name ? detected_name : "UNKNOWN");
    return 1;
}
#endif

//------------------------------ PUBLIC FUNCTIONS -----------------------------

extern "C" void app_main(void) {
    // Shared WiFi Initialization
    wifi_init();

#ifdef USE_CAMERA_NODE
    // ---------------------------------------------------------
    // CAMERA MODE LOGIC
    // ---------------------------------------------------------
    ESP_LOGI("APP", "Starting in CAMERA NODE mode");
    
    // Give WiFi 3 seconds to get an IP address before starting HTTP POSTs
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

    ui_register_sketchpad_sensor_color_callback(ui_request_sketchpad_sensor_color_from_bridge);
    gui_init();

    g_audio_queue = xQueueCreate(8, sizeof(audio_event_t));
    if (g_audio_queue != nullptr) {
        xTaskCreatePinnedToCore(audio_task, "audio_task", 4096, nullptr, 2, nullptr, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // Time for WiFi to connect
    init_time_sync();

    MQTTManager &mqtt = MQTTManager::get_instance();
    mqtt.connect("mqtt://172.16.55.93:1883");

    static MessengerApp app(mqtt, "kid");
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
                if (timeinfo.tm_yday != last_alarm_yday || timeinfo.tm_hour != last_alarm_hour || timeinfo.tm_min != last_alarm_minute) {
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