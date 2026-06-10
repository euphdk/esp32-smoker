#pragma once

#include <Preferences.h>

class Settings {
public:
  void begin();

  float targetC() const { return targetC_; }
  void setTargetC(float v);

  float calibrationC() const { return calibrationC_; }
  void setCalibrationC(float v);

  void flush(unsigned long nowMs);

private:
  Preferences prefs_;
  float targetC_ = 107.0f;
  float calibrationC_ = 0.0f;
  bool dirty_ = false;
  unsigned long lastWriteMs_ = 0;
  bool loaded_ = false;
};
