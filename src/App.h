#pragma once

#include "Controller.h"
#include "Outputs.h"
#include "Settings.h"
#include "SimulatedTemperatureSensor.h"
#include "TouchInput.h"
#include "Ui.h"

class App {
public:
  void begin();
  void loop();

private:
  Settings settings_;
  Ui ui_;
  TouchInput touch_;
  SimulatedTemperatureSensor sensor_;
  Controller controller_;
  Outputs outputs_;

  unsigned long lastSensorMs_ = 0;
  unsigned long lastControlMs_ = 0;
  unsigned long lastUiMs_ = 0;

  void handleTouch(unsigned long nowMs);
  void handleSerial();
};
