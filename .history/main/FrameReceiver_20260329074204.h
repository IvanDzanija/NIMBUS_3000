#pragma once
#include <string>

#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class FrameReceiver {
 private:
  std::string _server_url;  // e.g. "http://172.16.55.x/latest"
  static constexpr const char *TAG = "FrameReceiver";
  TaskHandle_t _task_handle = nullptr;

  // Buffer to receive JPEG into — 10KB fits QQVGA quality 63
  static constexpr size_t BUF_SIZE = 64 * 1024;
  uint8_t *_frame_buf = nullptr;
  size_t _frame_len = 0;
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

  static void task_wrapper(void *pv) { static_cast<FrameReceiver *>(pv)->run(); }

  // Called by HTTP client event handler to accumulate response body
  static esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    auto *self = static_cast<FrameReceiver *>(evt->user_data);
    switch (evt->event_id) {
      case HTTP_EVENT_ON_DATA:
        if (self->_frame_len + evt->data_len < BUF_SIZE) {
          memcpy(self->_frame_buf + self->_frame_len, evt->data, evt->data_len);
          self->_frame_len += evt->data_len;
        } else {
          ESP_LOGW(self->TAG, "Frame too large, dropping chunk");
        }
        break;
      default:
        break;
    }
    return ESP_OK;
  }

  void run() {
    _frame_buf = static_cast<uint8_t *>(
        heap_caps_malloc(BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!_frame_buf) {
      ESP_LOGW(TAG, "PSRAM unavailable, falling back to internal RAM");
      _frame_buf = static_cast<uint8_t *>(malloc(BUF_SIZE));
    }
    if (!_frame_buf) {
      ESP_LOGE(TAG, "Failed to alloc frame buffer");
      vTaskDelete(NULL);
      return;
    }

    esp_http_client_config_t config = {};
    config.url = _server_url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 3000;
    config.keep_alive_enable = true;
    config.event_handler = http_event_handler;
    config.user_data = this;

    esp_http_client_handle_t client = esp_http_client_init(&config);

    ESP_LOGI(TAG, "Starting receive loop from %s", _server_url.c_str());

    while (true) {
      _frame_len = 0;  // reset buffer for new frame

      esp_err_t err = esp_http_client_perform(client);

      if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status == 200 && _frame_len > 0) {
          // ── Frame is in _frame_buf, _frame_len bytes ──
          // Do whatever you need with it here:
          on_frame(_frame_buf, _frame_len);
        } else if (status == 503) {
          ESP_LOGW(TAG, "Server has no frame yet, waiting...");
          vTaskDelay(pdMS_TO_TICKS(200));
        }
      } else {
        ESP_LOGE(TAG, "GET failed: %s", esp_err_to_name(err));
        vTaskDelay(pdMS_TO_TICKS(500));
      }

      // Throttle to ~20 FPS max (50ms per frame)
      vTaskDelay(pdMS_TO_TICKS(50));
    }

    heap_caps_free(_frame_buf);
    esp_http_client_cleanup(client);
    vTaskDelete(NULL);
  }

 protected:
  // ── Override this to use the frame ──────────────────────────
  // Default just logs the size. Replace with display/CV/forward logic.
  virtual void on_frame(const uint8_t *buf, size_t len) {
    ESP_LOGI(TAG, "Got frame: %d bytes", len);
  }
  // ────────────────────────────────────────────────────────────

 public:
  explicit FrameReceiver(const std::string &url) : _server_url(url) {}

  void start(uint32_t stack_size = 8192, UBaseType_t priority = 5) {
    xTaskCreatePinnedToCore(task_wrapper, "recv_task", stack_size, this, priority,
                            &_task_handle, 1);
  }
};