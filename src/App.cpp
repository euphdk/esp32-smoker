#include "App.h"

#include <Arduino.h>

#include "Config.h"

void App::begin() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("\n[boot] %s firmware %s\n", Config::ProjectName, Config::FirmwareVersion);

  settings_.begin();
  outputs_.begin();
  sensor_.begin();
  controller_.begin(settings_);
  ui_.begin();
  touch_.begin();

  ui_.showBoot();
  delay(1200);
  ui_.drawMain(sensor_.currentC() + settings_.calibrationC(), controller_.status(), true);
}

void App::loop() {
  const unsigned long now = millis();
  handleSerial();
  handleTouch(now);
  settings_.flush(now);

  const float calC = settings_.calibrationC();
  const float pitC = sensor_.currentC() + calC;

  if (now - lastSensorMs_ >= Config::SensorUpdateMs) {
    lastSensorMs_ = now;
    const ControlStatus status = controller_.status();
    const float heatPercent = status.outputs.auger ? status.controlPercent : 0.0f;
    sensor_.update(now, heatPercent, status.outputs.fan);
    Serial.printf("[sensor] pit=%.1fC cal=%+.1fC valid=%d\n", pitC, calC, sensor_.valid());
  }

  if (now - lastControlMs_ >= Config::ControlUpdateMs) {
    lastControlMs_ = now;
    controller_.update(now, pitC, sensor_.valid());
    outputs_.apply(controller_.status().outputs);
  }

  if (now - lastUiMs_ >= Config::UiUpdateMs) {
    lastUiMs_ = now;
    const ControlStatus status = controller_.status();
    if (status.mode == SmokerMode::Error || status.mode == SmokerMode::ErrorCooldown) {
      ui_.drawError(pitC, status, controller_.errorCooldownElapsedMs(now));
    } else {
      ui_.drawMain(pitC, status);
    }
  }
}

void App::handleTouch(unsigned long nowMs) {
  int16_t x = 0;
  int16_t y = 0;
  if (!touch_.readPoint(x, y)) {
    return;
  }

  const SmokerMode mode = controller_.status().mode;
  const bool inFault = mode == SmokerMode::Error || mode == SmokerMode::ErrorCooldown;
  const UiAction action = ui_.actionForPoint(x, y);
  const float pitC = sensor_.currentC() + settings_.calibrationC();

  if (action == UiAction::AcknowledgeError) {
    if (mode == SmokerMode::Error) {
      controller_.acknowledgeError(nowMs);
      ui_.drawError(pitC, controller_.status(), 0, true);
    }
    return;
  }

  if (inFault || action == UiAction::None) {
    return;
  }

  switch (action) {
  case UiAction::IncreaseTarget:
    controller_.increaseTarget();
    break;
  case UiAction::DecreaseTarget:
    controller_.decreaseTarget();
    break;
  case UiAction::Start:
    controller_.start(nowMs);
    break;
  case UiAction::Stop:
    controller_.stop(nowMs);
    break;
  default:
    return;
  }
  ui_.drawMain(pitC, controller_.status(), true);
}

void App::handleSerial() {
  if (!Serial.available()) {
    return;
  }

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) {
    return;
  }

  if (line.startsWith("cal=")) {
    const char *value = line.c_str() + 4;
    char *end = nullptr;
    const float parsed = strtof(value, &end);
    if (end == value) {
      Serial.println("[settings] cal= requires a numeric value, e.g. cal=1.5");
      return;
    }
    if (parsed < Config::CalibrationMinC || parsed > Config::CalibrationMaxC) {
      Serial.printf("[settings] cal=%.2f out of range [%.1f, %.1f]\n",
                    parsed, Config::CalibrationMinC, Config::CalibrationMaxC);
      return;
    }
    settings_.setCalibrationC(parsed);
    Serial.printf("[settings] cal=%+.1fC pending save\n", settings_.calibrationC());
    return;
  }

  Serial.printf("[settings] unknown command: %s\n", line.c_str());
}
