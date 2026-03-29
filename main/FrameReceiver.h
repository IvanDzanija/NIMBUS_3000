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
  volatile bool _running = false;
  uint32_t _no_frame_log_divider = 0;

  // Keep this small enough for boards without PSRAM.
  static constexpr size_t BUF_SIZE = 12 * 1024;
  uint8_t *_frame_buf = nullptr;
  size_t _frame_len = 0;
  portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;

  static void task_wrapper(void *pv) { static_cast<FrameReceiver *>(pv)->run(); }

  esp_http_client_handle_t create_client() {
    esp_http_client_config_t config = {};
    config.url = _server_url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 3000;
    config.keep_alive_enable = false;
    config.event_handler = http_event_handler;
    config.user_data = this;
    return esp_http_client_init(&config);
  }

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

    esp_http_client_handle_t client = create_client();

    ESP_LOGI(TAG, "Starting receive loop from %s", _server_url.c_str());

    while (_running) {
      if (client == nullptr) {
        client = create_client();
        if (client == nullptr) {
          ESP_LOGW(TAG, "HTTP client recreate failed");
          vTaskDelay(pdMS_TO_TICKS(1000));
          continue;
        }
      }

      _frame_len = 0;  // reset buffer for new frame

      esp_err_t err = esp_http_client_perform(client);

      if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status == 200 && _frame_len > 0) {
          _no_frame_log_divider = 0;
          // ── Frame is in _frame_buf, _frame_len bytes ──
          // Do whatever you need with it here:
          on_frame(_frame_buf, _frame_len);
        } else if (status == 503) {
          _no_frame_log_divider++;
          if ((_no_frame_log_divider % 20) == 1) {
            ESP_LOGW(TAG, "Server jos nema frame, cekam...");
          }
          vTaskDelay(pdMS_TO_TICKS(200));
        }
      } else {
        ESP_LOGW(TAG, "Frame request nije uspio: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        client = nullptr;
        vTaskDelay(pdMS_TO_TICKS(500));
      }

      // Throttle to ~20 FPS max (50ms per frame)
      vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (_frame_buf) {
      heap_caps_free(_frame_buf);
      _frame_buf = nullptr;
    }
    if (client) {
      esp_http_client_cleanup(client);
    }
    _task_handle = nullptr;
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
    if (_task_handle != nullptr) return;
    _running = true;
    xTaskCreatePinnedToCore(task_wrapper, "recv_task", stack_size, this, priority,
                            &_task_handle, 1);
  }

  void stop(TickType_t wait_ticks = pdMS_TO_TICKS(4000)) {
    if (_task_handle == nullptr) return;
    ESP_LOGI(TAG, "Stopping frame receiver");
    _running = false;

    TickType_t start = xTaskGetTickCount();
    while (_task_handle != nullptr &&
           (xTaskGetTickCount() - start) < wait_ticks) {
      vTaskDelay(pdMS_TO_TICKS(50));
    }

    if (_task_handle == nullptr) {
      ESP_LOGI(TAG, "Frame receiver stopped");
    } else {
      ESP_LOGW(TAG, "Frame receiver stop timed out");
    }
  }
};
