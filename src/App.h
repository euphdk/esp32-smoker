#pragma once

#include "Controller.h"
#include "LogBuffer.h"
#include "Network.h"
#include "Outputs.h"
#include "Settings.h"
#include "SimulatedTemperatureSensor.h"
#include "TouchInput.h"
#include "Ui.h"

class App {
public:
  void begin();
  void loop();

  void setMqttCommandHandlers(void (*setTarget)(float), void (*setCalibration)(float), void (*ackError)(unsigned long));

  void setTarget(float v) { controller_.setTarget(v); }
  void setCalibration(float v) { settings_.setCalibrationC(v); }
  void acknowledgeError(unsigned long nowMs) { controller_.acknowledgeError(nowMs); }

private:
  LogBuffer logBuffer_;
  Settings settings_;
  Ui ui_;
  TouchInput touch_;
  SimulatedTemperatureSensor sensor_;
  Controller controller_;
  Outputs outputs_;
  Network network_;

  unsigned long lastSensorMs_ = 0;
  unsigned long lastControlMs_ = 0;
  unsigned long lastUiMs_ = 0;

  void handleTouch(unsigned long nowMs);
  void handleSerial();
};
