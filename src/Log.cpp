#include "Log.h"

#include <Arduino.h>

MqttSerial Log;

void MqttSerial::begin(unsigned long baud) {
  Serial.begin(baud);
}

void MqttSerial::setBackend(LogBackend *backend) {
  backend_ = backend;
}

size_t MqttSerial::write(uint8_t c) {
  const size_t n = Serial.write(c);
  if (backend_ != nullptr) {
    if (c == '\n') {
      inLine_ = false;
      backend_->lineComplete();
    } else if (!inLine_) {
      inLine_ = true;
      backend_->lineStart();
    }
    if (inLine_) {
      backend_->lineByte(static_cast<char>(c));
    }
  }
  return n;
}

size_t MqttSerial::write(const uint8_t *buffer, size_t size) {
  size_t total = 0;
  for (size_t i = 0; i < size; ++i) {
    total += write(buffer[i]);
  }
  return total;
}
