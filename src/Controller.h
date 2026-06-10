#pragma once

#include "Types.h"

class Controller {
public:
  void begin();
  void update(unsigned long nowMs, float pitTempC, bool sensorValid);
  void start(unsigned long nowMs);
  void stop(unsigned long nowMs);
  void increaseTarget();
  void decreaseTarget();
  ControlStatus status() const;

private:
  ControlStatus status_;
  unsigned long modeStartedMs_ = 0;
  unsigned long lastControlLogMs_ = 0;

  void enterMode(SmokerMode mode, unsigned long nowMs, const char *errorMessage = nullptr);
  void calculateControl(float pitTempC);
  void updateOutputs(unsigned long nowMs);
  void fail(unsigned long nowMs, const char *message);
};
