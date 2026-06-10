#include "Ui.h"

#include <Arduino.h>

#include "Config.h"

namespace {
constexpr int16_t MinusX = 12;
constexpr int16_t PlusX = 128;
constexpr int16_t TargetY = 128;
constexpr int16_t ButtonW = 100;
constexpr int16_t ButtonH = 42;
constexpr int16_t StartX = 12;
constexpr int16_t StopX = 128;
constexpr int16_t StartStopY = 264;
} // namespace

void Ui::begin() {
  tft_.init();
  tft_.setRotation(0);
  tft_.fillScreen(TFT_BLACK);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
}

void Ui::showBoot() {
  tft_.fillScreen(TFT_NAVY);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(TFT_WHITE, TFT_NAVY);
  tft_.drawString(Config::ProjectName, Config::ScreenWidth / 2, 118, 4);
  tft_.drawString(String("FW ") + Config::FirmwareVersion, Config::ScreenWidth / 2, 162, 2);
}

void Ui::drawMain(float pitTempC, const ControlStatus &status, bool force) {
  const bool changed = force || !drawn_ || abs(lastPitTempC_ - pitTempC) >= 0.1f ||
                       abs(lastTargetC_ - status.targetC) >= 0.1f || lastMode_ != status.mode ||
                       lastOutputs_.auger != status.outputs.auger || lastOutputs_.fan != status.outputs.fan ||
                       lastOutputs_.igniter != status.outputs.igniter;
  if (!changed) {
    return;
  }

  drawn_ = true;
  lastPitTempC_ = pitTempC;
  lastTargetC_ = status.targetC;
  lastMode_ = status.mode;
  lastOutputs_ = status.outputs;

  tft_.fillScreen(TFT_BLACK);
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextColor(TFT_CYAN, TFT_BLACK);
  tft_.drawString(Config::ProjectName, 10, 8, 2);

  char value[32];
  snprintf(value, sizeof(value), "%.1f C", pitTempC);
  drawStatusLine(38, "Pit", value, TFT_WHITE);

  snprintf(value, sizeof(value), "%.0f C", status.targetC);
  drawStatusLine(76, "Target", value, TFT_YELLOW);

  drawButton(MinusX, TargetY, ButtonW, ButtonH, "-5 C", TFT_DARKGREY);
  drawButton(PlusX, TargetY, ButtonW, ButtonH, "+5 C", TFT_DARKGREY);

  drawStatusLine(184, "Mode", modeToString(status.mode), status.mode == SmokerMode::Error ? TFT_RED : TFT_GREEN);
  drawOutputState(214, "Auger", status.outputs.auger);
  drawOutputState(234, "Fan", status.outputs.fan);
  drawOutputState(254, "Igniter", status.outputs.igniter);

  drawButton(StartX, StartStopY, ButtonW, ButtonH, "Start", TFT_DARKGREEN);
  drawButton(StopX, StartStopY, ButtonW, ButtonH, "Stop", TFT_MAROON);

  if (status.errorMessage != nullptr) {
    tft_.setTextColor(TFT_RED, TFT_BLACK);
    tft_.drawString(status.errorMessage, 10, 308, 2);
  }
}

UiAction Ui::actionForPoint(int16_t x, int16_t y) const {
  if (y >= TargetY && y <= TargetY + ButtonH) {
    if (x >= MinusX && x <= MinusX + ButtonW) {
      return UiAction::DecreaseTarget;
    }
    if (x >= PlusX && x <= PlusX + ButtonW) {
      return UiAction::IncreaseTarget;
    }
  }

  if (y >= StartStopY && y <= StartStopY + ButtonH) {
    if (x >= StartX && x <= StartX + ButtonW) {
      return UiAction::Start;
    }
    if (x >= StopX && x <= StopX + ButtonW) {
      return UiAction::Stop;
    }
  }

  return UiAction::None;
}

void Ui::drawButton(int16_t x, int16_t y, int16_t w, int16_t h, const char *label, uint16_t color) {
  tft_.fillRoundRect(x, y, w, h, 6, color);
  tft_.drawRoundRect(x, y, w, h, 6, TFT_WHITE);
  tft_.setTextDatum(MC_DATUM);
  tft_.setTextColor(TFT_WHITE, color);
  tft_.drawString(label, x + w / 2, y + h / 2, 2);
  tft_.setTextDatum(TL_DATUM);
}

void Ui::drawStatusLine(int16_t y, const char *label, const char *value, uint16_t valueColor) {
  tft_.setTextDatum(TL_DATUM);
  tft_.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft_.drawString(label, 10, y, 2);
  tft_.setTextColor(valueColor, TFT_BLACK);
  tft_.drawString(value, 96, y, 4);
}

void Ui::drawOutputState(int16_t y, const char *label, bool on) {
  tft_.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft_.drawString(label, 10, y, 2);
  tft_.setTextColor(on ? TFT_GREEN : TFT_DARKGREY, TFT_BLACK);
  tft_.drawString(on ? "ON" : "OFF", 96, y, 2);
}
