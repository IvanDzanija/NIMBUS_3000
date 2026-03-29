#pragma once
#include <functional>
#include <string>
#include <vector>

#include "IMQTTListener.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

class MQTTManager {
 private:
  static constexpr const char *TAG = "MQTT_ARCH";
  static constexpr EventBits_t CONNECTED_BIT = BIT0;

  esp_mqtt_client_handle_t _client = nullptr;
  std::vector<IMQTTListener *> _listeners;
  std::vector<std::string> _pending_subscriptions;
  EventGroupHandle_t _event_group;

  MQTTManager() { _event_group = xEventGroupCreate(); }

 public:
  static MQTTManager &get_instance() {
    static MQTTManager instance;
    return instance;
  }

  void connect(const char *uri) {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = uri;
    _client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
                                   _mqtt_event_handler, this);
    esp_mqtt_client_start(_client);

    ESP_LOGI(TAG, "Waiting for MQTT connection...");
    xEventGroupWaitBits(_event_group, CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "MQTT ready!");
  }

  void subscribe(const std::string &topic) {
    _pending_subscriptions.push_back(topic);
    if (_client) {
      esp_mqtt_client_subscribe(_client, topic.c_str(), 1);
    }
  }

  void add_listener(IMQTTListener *app) {
    _listeners.push_back(app);
    if (_client && app->get_topic() != Topic::UNKNOWN) {
      esp_mqtt_client_subscribe(_client, topic_to_string(app->get_topic()).c_str(), 1);
    }
  }

  void publish(const std::string &topic, const std::string &message) {
    if (_client)
      esp_mqtt_client_publish(_client, topic.c_str(), message.c_str(), 0, 1, 0);
  }

  MQTTManager(const MQTTManager &) = delete;
  void operator=(const MQTTManager &) = delete;

 private:
  static void _mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                  int32_t event_id, void *event_data) {
    auto *manager = static_cast<MQTTManager *>(handler_args);
    auto event = (esp_mqtt_event_handle_t)event_data;

    if (event_id == MQTT_EVENT_CONNECTED) {
      ESP_LOGI(TAG, "Connected — subscribing to %d topics",
               manager->_pending_subscriptions.size());

      for (const auto &topic : manager->_pending_subscriptions) {
        esp_mqtt_client_subscribe(manager->_client, topic.c_str(), 1);
        ESP_LOGI(TAG, "Subscribed: %s", topic.c_str());
      }

      xEventGroupSetBits(manager->_event_group, CONNECTED_BIT);
    }

    if (event_id == MQTT_EVENT_DATA) {
      std::string raw_topic(event->topic, event->topic_len);
      std::string payload(event->data, event->data_len);
      Topic incoming = string_to_topic(raw_topic);
      for (IMQTTListener *listener : manager->_listeners) {
        if (listener->matches_topic(raw_topic, incoming)) {
          listener->on_message_received(raw_topic, payload);
        }
      }
    }
  }
};