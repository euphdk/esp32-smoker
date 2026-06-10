#pragma once

#include <XPT2046_Touchscreen.h>

class TouchInput {
public:
  TouchInput();
  void begin();
  bool readPoint(int16_t &x, int16_t &y);

private:
  XPT2046_Touchscreen touch_;
  unsigned long lastTouchMs_ = 0;
};
