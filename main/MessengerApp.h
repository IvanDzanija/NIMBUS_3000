#pragma once
#include "ChatManager.h"
#include "MQTTManager.h"
#include "ui_app/ui_message_bridge.h"

class MessengerApp {
 public:
  MessengerApp(MQTTManager &mqtt, const std::string &my_id)
      : _mqtt(mqtt), _chat(my_id, mqtt) {}

  void begin() {
    // Register known parents
    _chat.add_parent("Mom");
    _chat.add_parent("Dad");

    if (auto *s = _chat.get_session("Mom")) {
      s->on_receive = [](const Message &msg) {
        ESP_LOGI("APP", "Mom says: %s", msg.content.c_str());
        ui_store_received_message("Mama", msg.content.c_str());
        ui_request_message_sound();
      };
    }
    if (auto *s = _chat.get_session("Dad")) {
      s->on_receive = [](const Message &msg) {
        ESP_LOGI("APP", "Dad says: %s", msg.content.c_str());
        ui_store_received_message("Tata", msg.content.c_str());
        ui_request_message_sound();
      };
    }
  }

  void send_to_parent(const std::string &parent_id, const std::string &msg) {
    _chat.send_to(parent_id, msg);
  }

 private:
  MQTTManager &_mqtt;
  ChatManager _chat;
};
