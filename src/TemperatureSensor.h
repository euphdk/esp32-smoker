#pragma once

class TemperatureSensor {
public:
  virtual ~TemperatureSensor() = default;
  virtual void begin() = 0;
  virtual void update(unsigned long nowMs, float heatPercent, bool fanOn) = 0;
  virtual float currentC() const = 0;
  virtual bool valid() const = 0;
};
