#pragma once
#include "Message.h"

class IMessageHandler {
 public:
  virtual void on_message(const Message &msg) = 0;
  virtual ~IMessageHandler() = default;
};