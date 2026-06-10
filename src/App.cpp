#include "App.h"

#include <Arduino.h>

#include "Config.h"

void App::begin() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("\n[boot] %s firmware %s\n", Config::ProjectName, Config::FirmwareVersion);

  outputs_.begin();
  sensor_.begin();
  controller_.begin();
  ui_.begin();
  touch_.begin();

  ui_.showBoot();
  delay(1200);
  ui_.drawMain(sensor_.currentC(), controller_.status(), true);
}

void App::loop() {
  const unsigned long now = millis();
  handleTouch(now);

  if (now - lastSensorMs_ >= Config::SensorUpdateMs) {
    lastSensorMs_ = now;
    const ControlStatus status = controller_.status();
    const float heatPercent = status.outputs.auger ? status.controlPercent : 0.0f;
    sensor_.update(now, heatPercent, status.outputs.fan);
    Serial.printf("[sensor] pit=%.1fC valid=%d\n", sensor_.currentC(), sensor_.valid());
  }

  if (now - lastControlMs_ >= Config::ControlUpdateMs) {
    lastControlMs_ = now;
    controller_.update(now, sensor_.currentC(), sensor_.valid());
    outputs_.apply(controller_.status().outputs);
  }

  if (now - lastUiMs_ >= Config::UiUpdateMs) {
    lastUiMs_ = now;
    const ControlStatus status = controller_.status();
    if (status.mode == SmokerMode::Error || status.mode == SmokerMode::ErrorCooldown) {
      ui_.drawError(sensor_.currentC(), status, controller_.errorCooldownElapsedMs(now));
    } else {
      ui_.drawMain(sensor_.currentC(), status);
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

  if (action == UiAction::AcknowledgeError) {
    if (mode == SmokerMode::Error) {
      controller_.acknowledgeError(nowMs);
      ui_.drawError(sensor_.currentC(), controller_.status(), 0, true);
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
  ui_.drawMain(sensor_.currentC(), controller_.status(), true);
}
