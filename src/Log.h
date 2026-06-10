#pragma once

#include <Arduino.h>

class LogBackend {
public:
  virtual void lineStart() = 0;
  virtual void lineByte(char c) = 0;
  virtual void lineComplete() = 0;
  virtual ~LogBackend() = default;
};

class MqttSerial : public Print {
public:
  void begin(unsigned long baud);
  void setBackend(LogBackend *backend);
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buffer, size_t size) override;
  using Print::printf;
  using Print::print;
  using Print::println;

private:
  LogBackend *backend_ = nullptr;
  bool inLine_ = false;
};

extern MqttSerial Log;
