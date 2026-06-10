#include "App.h"

#include <Arduino.h>

#include "Config.h"
#include "Log.h"

void App::begin() {
  Log.begin(115200);
  delay(100);
  logBuffer_.begin();
  Log.printf("\n[boot] %s firmware %s\n", Config::ProjectName, Config::FirmwareVersion);

  settings_.begin();
  outputs_.begin();
  sensor_.begin();
  controller_.begin(settings_);
  ui_.begin();
  touch_.begin();
  network_.begin(settings_);
  web_.begin(controller_);

  ui_.showBoot();
  delay(1200);
  ui_.drawMain(sensor_.currentC() + settings_.calibrationC(), controller_.status(), true);
}

void App::setMqttCommandHandlers(void (*setTarget)(float), void (*setCalibration)(float), void (*ackError)(unsigned long)) {
  network_.setCommandTarget(setTarget);
  network_.setCommandCalibration(setCalibration);
  network_.setCommandAck(ackError);
}

void App::loop() {
  const unsigned long now = millis();
  handleSerial();
  handleTouch(now);
  network_.loop();
  settings_.flush(now);

  const float calC = settings_.calibrationC();
  const float pitC = sensor_.currentC() + calC;

  if (now - lastSensorMs_ >= Config::SensorUpdateMs) {
    lastSensorMs_ = now;
    const ControlStatus status = controller_.status();
    const float heatPercent = status.outputs.auger ? status.controlPercent : 0.0f;
    sensor_.update(now, heatPercent, status.outputs.fan);
    Log.printf("[sensor] pit=%.1fC cal=%+.1fC valid=%d\n", pitC, calC, sensor_.valid());
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

  StatusSnapshot snap;
  snap.pitC = pitC;
  snap.targetC = controller_.status().targetC;
  snap.calibrationC = calC;
  snap.mode = controller_.status().mode;
  snap.outputs = controller_.status().outputs;
  snap.wifiConnected = network_.wifiConnected();
  snap.rssi = network_.rssi();
  snap.ip = network_.ipAddress();
  snap.errorMessage = controller_.status().errorMessage;
  snap.uptimeMs = now;
  network_.publishStatus(snap);
  web_.updateSnapshot(snap);
  network_.drainLogs(logBuffer_);
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
      Log.println("[settings] cal= requires a numeric value, e.g. cal=1.5");
      return;
    }
    if (parsed < Config::CalibrationMinC || parsed > Config::CalibrationMaxC) {
      Log.printf("[settings] cal=%.2f out of range [%.1f, %.1f]\n",
                 parsed, Config::CalibrationMinC, Config::CalibrationMaxC);
      return;
    }
    settings_.setCalibrationC(parsed);
    Log.printf("[settings] cal=%+.1fC pending save\n", settings_.calibrationC());
    return;
  }

  if (line.startsWith("wifi_ssid=")) {
    settings_.setWifiSsid(line.substring(10));
    Log.printf("[settings] wifi_ssid=%s pending save\n", settings_.wifiSsid().c_str());
    return;
  }

  if (line.startsWith("wifi_pass=")) {
    settings_.setWifiPass(line.substring(10));
    Log.println("[settings] wifi_pass=*** pending save");
    return;
  }

  if (line.startsWith("mqtt_host=")) {
    settings_.setMqttHost(line.substring(10));
    Log.printf("[settings] mqtt_host=%s pending save\n", settings_.mqttHost().c_str());
    return;
  }

  if (line.startsWith("mqtt_port=")) {
    const char *value = line.c_str() + 10;
    char *end = nullptr;
    const long parsed = strtol(value, &end, 10);
    if (end == value || parsed < 1 || parsed > 65535) {
      Log.println("[settings] mqtt_port= requires 1..65535");
      return;
    }
    settings_.setMqttPort(static_cast<uint16_t>(parsed));
    Log.printf("[settings] mqtt_port=%u pending save\n", settings_.mqttPort());
    return;
  }

  if (line.startsWith("mqtt_user=")) {
    settings_.setMqttUser(line.substring(10));
    Log.printf("[settings] mqtt_user=%s pending save\n", settings_.mqttUser().c_str());
    return;
  }

  if (line.startsWith("mqtt_pass=")) {
    settings_.setMqttPass(line.substring(10));
    Log.println("[settings] mqtt_pass=*** pending save");
    return;
  }

  if (line.startsWith("mqtt_base=")) {
    settings_.setMqttBaseTopic(line.substring(10));
    Log.printf("[settings] mqtt_base=%s pending save\n", settings_.mqttBaseTopic().c_str());
    return;
  }

  Log.printf("[settings] unknown command: %s\n", line.c_str());
}
