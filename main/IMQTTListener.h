#ifndef IMQTTLISTENER_H
#define IMQTTLISTENER_H
#include <cstdint>
#include <string>

enum class Topic : uint8_t { MESSAGE, STATUS, SYSTEM, UNKNOWN };

inline std::string topic_to_string(Topic t) {
  switch (t) {
    case Topic::MESSAGE:
      return "room/messages";
    case Topic::STATUS:
      return "room/status";
    case Topic::SYSTEM:
      return "room/sys";
    default:
      return "unknown/topic";
  }
}

inline Topic string_to_topic(const std::string &s) {
  if (s == "room/messages") return Topic::MESSAGE;
  if (s == "room/status") return Topic::STATUS;
  if (s == "room/sys") return Topic::SYSTEM;
  return Topic::UNKNOWN;
}

class IMQTTListener {
 public:
  virtual Topic get_topic() const = 0;
  virtual void on_message_received(const std::string &topic,
                                   const std::string &payload) = 0;

  virtual bool matches_topic(const std::string &raw_topic, Topic resolved) const {
    return get_topic() == resolved;
  }

  virtual ~IMQTTListener() = default;
};

#endif