#include "SimulatedTemperatureSensor.h"

#include <Arduino.h>

#include "Config.h"

void SimulatedTemperatureSensor::begin() {
  temperatureC_ = 22.0f;
  lastUpdateMs_ = millis();
}

void SimulatedTemperatureSensor::update(unsigned long nowMs, float heatPercent, bool fanOn) {
  if (lastUpdateMs_ == 0) {
    lastUpdateMs_ = nowMs;
    return;
  }

  const float dtSeconds = static_cast<float>(nowMs - lastUpdateMs_) / 1000.0f;
  lastUpdateMs_ = nowMs;

  const float ambientC = 22.0f;
  const float heatRate = heatPercent * 0.015f;
  const float fanCooling = fanOn ? 0.025f : 0.010f;
  const float coolingRate = (temperatureC_ - ambientC) * fanCooling;

  temperatureC_ += (heatRate - coolingRate) * dtSeconds;
  temperatureC_ = constrain(temperatureC_, ambientC, 260.0f);
}

float SimulatedTemperatureSensor::currentC() const { return temperatureC_; }

bool SimulatedTemperatureSensor::valid() const {
  return temperatureC_ >= Config::InvalidLowC && temperatureC_ <= Config::InvalidHighC;
}
