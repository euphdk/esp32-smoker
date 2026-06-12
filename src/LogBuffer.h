#pragma once

#include "Config.h"
#include "Log.h"

class LogBuffer : public LogBackend {
public:
  void begin();

  void lineStart() override;
  void lineByte(char c) override;
  void lineComplete() override;

  bool takeLine(char *out, size_t outSize);

  size_t pending() const { return count_; }

private:
  char lines_[Config::LogRingCapacity][Config::LogLineMaxLen];
  size_t head_ = 0;
  size_t tail_ = 0;
  size_t count_ = 0;
  char current_[Config::LogLineMaxLen];
  size_t currentLen_ = 0;
};
