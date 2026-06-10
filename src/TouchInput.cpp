#include "TouchInput.h"

#include <Arduino.h>

#include "Config.h"
#include "Log.h"

TouchInput::TouchInput() : touch_(Config::TouchCsPin, Config::TouchIrqPin) {}

void TouchInput::begin() { touch_.begin(); }

bool TouchInput::readPoint(int16_t &x, int16_t &y) {
  const unsigned long now = millis();
  if (now - lastTouchMs_ < 200 || !touch_.touched()) {
    return false;
  }

  TS_Point point = touch_.getPoint();
  x = map(point.x, Config::TouchMinX, Config::TouchMaxX, 0, Config::ScreenWidth);
  y = map(point.y, Config::TouchMinY, Config::TouchMaxY, 0, Config::ScreenHeight);
  x = constrain(x, 0, Config::ScreenWidth - 1);
  y = constrain(y, 0, Config::ScreenHeight - 1);

  lastTouchMs_ = now;
  Log.printf("[touch] x=%d y=%d raw=(%d,%d)\n", x, y, point.x, point.y);
  return true;
}
