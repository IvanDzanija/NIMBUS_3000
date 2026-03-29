#pragma once
#include <string>
#include <vector>

#include "esp_camera.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27
#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

class CameraStreamer {
 private:
  std::string _server_url;
  static constexpr const char *TAG = "CameraStreamer";
  TaskHandle_t _task_handle = nullptr;

  camera_config_t _camera_config = {
      .pin_pwdn = CAM_PIN_PWDN,
      .pin_reset = CAM_PIN_RESET,
      .pin_xclk = CAM_PIN_XCLK,
      .pin_sccb_sda = CAM_PIN_SIOD,
      .pin_sccb_scl = CAM_PIN_SIOC,
      .pin_d7 = CAM_PIN_D7,
      .pin_d6 = CAM_PIN_D6,
      .pin_d5 = CAM_PIN_D5,
      .pin_d4 = CAM_PIN_D4,
      .pin_d3 = CAM_PIN_D3,
      .pin_d2 = CAM_PIN_D2,
      .pin_d1 = CAM_PIN_D1,
      .pin_d0 = CAM_PIN_D0,
      .pin_vsync = CAM_PIN_VSYNC,
      .pin_href = CAM_PIN_HREF,
      .pin_pclk = CAM_PIN_PCLK,
      .xclk_freq_hz = 20000000,
      .ledc_timer = LEDC_TIMER_0,
      .ledc_channel = LEDC_CHANNEL_0,
      .pixel_format = PIXFORMAT_JPEG,
      .frame_size = FRAMESIZE_QQVGA,
      .jpeg_quality = 10,   // 0=best, 63=worst — was 63
      .fb_count = 2,
      .fb_location = CAMERA_FB_IN_PSRAM,
      .grab_mode = CAMERA_GRAB_LATEST,  // always newest frame
  };

  static void stream_task_wrapper(void *pvParameters) {
    static_cast<CameraStreamer *>(pvParameters)->run_stream();
  }

  void run_stream() {
    esp_http_client_config_t config = {};
    config.url = _server_url.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 5000;
    config.disable_auto_redirect = true;
    config.keep_alive_enable = true;  // reuse TCP connection every frame

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "image/jpeg");

    // Pre-allocate a copy buffer once — avoids malloc every frame
    // QQVGA JPEG at quality 63 is well under 10 KB
    const size_t BUF_SIZE = 10 * 1024;
    uint8_t *jpg_copy = static_cast<uint8_t *>(
        heap_caps_malloc(BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (!jpg_copy) {
      ESP_LOGE(TAG, "Failed to allocate frame copy buffer");
      vTaskDelete(NULL);
      return;
    }

    ESP_LOGI(TAG, "Starting stream loop...");

    while (true) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        vTaskDelay(pdMS_TO_TICKS(100));
        continue;
      }

      // ── KEY CHANGE ──────────────────────────────────────────────
      // 1. Copy JPEG bytes into our own buffer
      size_t jpg_len = fb->len;
      memcpy(jpg_copy, fb->buf, jpg_len);

      // 2. Return the frame buffer immediately so the camera
      //    starts capturing the NEXT frame while we POST this one
      esp_camera_fb_return(fb);
      // ────────────────────────────────────────────────────────────

      // 3. POST the copied data — camera is already working in parallel
      esp_http_client_set_post_field(client, reinterpret_cast<const char *>(jpg_copy),
                                     jpg_len);

      esp_err_t err = esp_http_client_perform(client);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP POST failed: %s", esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(500));
      }

      // No delay needed — GRAB_LATEST + fb_count=2 handles pacing
    }

    heap_caps_free(jpg_copy);
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
  }

 public:
  explicit CameraStreamer(const std::string &url) : _server_url(url) {}

  esp_err_t init() {
    esp_err_t err = esp_camera_init(&_camera_config);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Camera Init Failed");
      return err;
    }
    return ESP_OK;
  }

  void start(uint32_t stack_size = 8192, UBaseType_t priority = 5) {
    xTaskCreatePinnedToCore(stream_task_wrapper, "stream_task", stack_size, this,
                            priority, &_task_handle, 1);
  }
};