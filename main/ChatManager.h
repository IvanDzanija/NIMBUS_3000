#pragma once
#include <map>
#include <memory>
#include <string>

#include "ChatSession.h"
#include "IMQTTListener.h"
#include "MQTTManager.h"

class ChatManager : public IMQTTListener {
 public:
  explicit ChatManager(const std::string &my_id, MQTTManager &mqtt)
      : _id(my_id), _mqtt(mqtt) {
    _mqtt.add_listener(this);
  }

  bool matches_topic(const std::string &raw_topic, Topic /*resolved*/) const override {
    return raw_topic.rfind("chat/", 0) == 0;
  }
  // Register a parent before starting
  void add_parent(const std::string &parent_id) {
    auto session = std::make_unique<ChatSession>(_id, parent_id, _mqtt);

    _mqtt.subscribe(session->inbox_topic());

    _sessions[parent_id] = std::move(session);
    ESP_LOGI("ChatManager", "Added parent: %s", parent_id.c_str());
  }

  void send_to(const std::string &parent_id, const std::string &msg) {
    auto it = _sessions.find(parent_id);
    if (it != _sessions.end()) {
      it->second->send(msg);
    } else {
      ESP_LOGW("ChatManager", "No session for parent: %s", parent_id.c_str());
    }
  }

  ChatSession *get_session(const std::string &parent_id) {
    auto it = _sessions.find(parent_id);
    return (it != _sessions.end()) ? it->second.get() : nullptr;
  }

  // IMQTTListener
  Topic get_topic() const override { return Topic::UNKNOWN; }

  void on_message_received(const std::string &topic,
                           const std::string &payload) override {
    // topic = "chat/{sender}/{receiver}"
    if (topic.rfind("chat/", 0) != 0) return;

    Message msg = Message::from_mqtt(topic, payload);

    // Only handle messages addressed to me
    if (msg.receiver_id != _id) return;

    auto it = _sessions.find(msg.sender_id);
    if (it != _sessions.end()) {
      it->second->deliver(msg);
    } else {
      ESP_LOGW("ChatManager", "Message from unknown sender: %s", msg.sender_id.c_str());
    }
  }

 private:
  std::string _id;
  MQTTManager &_mqtt;
  std::map<std::string, std::unique_ptr<ChatSession>> _sessions;
};