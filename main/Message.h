#ifndef MESSAGE_H
#define MESSAGE_H

#pragma once
#include <cstdint>
#include <string>

struct Message {
  std::string sender_id;
  std::string receiver_id;
  std::string content;
  uint64_t timestamp_ms;

  // Reconstruct topic
  std::string to_topic() const { return "chat/" + sender_id + "/" + receiver_id; }

  // Parse topic "chat/sender/receiver" back into a Message
  static Message from_mqtt(const std::string &topic, const std::string &payload) {
    Message m;
    // topic format: chat/{sender}/{receiver}
    auto first = topic.find('/', 5);  // skip "chat/"
    m.sender_id = topic.substr(5, first - 5);
    m.receiver_id = topic.substr(first + 1);
    m.content = payload;
    m.timestamp_ms = 0;
    return m;
  }
};

#endif