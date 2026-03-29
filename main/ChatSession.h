#pragma once
#include <functional>
#include <string>

#include "MQTTManager.h"
#include "Message.h"

class ChatSession {
 public:
  ChatSession(const std::string &my_id, const std::string &peer_id, MQTTManager &mqtt)
      : my_id_(my_id), peer_id_(peer_id), mqtt_(mqtt) {}

  // Topic this session SENDS on
  std::string outbox_topic() const { return "chat/" + my_id_ + "/" + peer_id_; }

  // Topic this session RECEIVES on
  std::string inbox_topic() const { return "chat/" + peer_id_ + "/" + my_id_; }

  const std::string &peer_id() const { return peer_id_; }

  void send(const std::string &content) {
    Message m;
    m.sender_id = my_id_;
    m.receiver_id = peer_id_;
    m.content = content;
    mqtt_.publish(outbox_topic(), content);
    ESP_LOGI("ChatSession", "[->%s] %s", peer_id_.c_str(), content.c_str());
  }

  void deliver(const Message &msg) {
    // Called by ChatManager when a message arrives for this session
    ESP_LOGI("ChatSession", "[%s->] %s", msg.sender_id.c_str(), msg.content.c_str());
    if (on_receive) on_receive(msg);
  }

  // Optional callback — set this to hook into your UI / display
  std::function<void(const Message &)> on_receive;

 private:
  std::string my_id_;
  std::string peer_id_;
  MQTTManager &mqtt_;
};