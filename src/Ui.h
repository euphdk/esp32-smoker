#pragma once

#include <TFT_eSPI.h>

#include "Types.h"

enum class UiAction {
  None,
  IncreaseTarget,
  DecreaseTarget,
  Start,
  Stop,
};

class Ui {
public:
  void begin();
  void showBoot();
  void drawMain(float pitTempC, const ControlStatus &status, bool force = false);
  UiAction actionForPoint(int16_t x, int16_t y) const;

private:
  TFT_eSPI tft_;
  float lastPitTempC_ = -1000.0f;
  float lastTargetC_ = -1000.0f;
  SmokerMode lastMode_ = SmokerMode::Error;
  OutputState lastOutputs_;
  bool drawn_ = false;

  void drawButton(int16_t x, int16_t y, int16_t w, int16_t h, const char *label, uint16_t color);
  void drawStatusLine(int16_t y, const char *label, const char *value, uint16_t valueColor);
  void drawOutputState(int16_t y, const char *label, bool on);
};
