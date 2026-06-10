#pragma once

#include "TemperatureSensor.h"

class SimulatedTemperatureSensor : public TemperatureSensor {
public:
  void begin() override;
  void update(unsigned long nowMs, float heatPercent, bool fanOn) override;
  float currentC() const override;
  bool valid() const override;

private:
  float temperatureC_ = 22.0f;
  unsigned long lastUpdateMs_ = 0;
};
