#pragma once

#include "Types.h"

class Settings;

class Controller {
public:
  Controller() = default;
  void begin(Settings &settings);

  void update(unsigned long nowMs, float pitTempC, bool sensorValid);
  void start(unsigned long nowMs);
  void stop(unsigned long nowMs);
  void acknowledgeError(unsigned long nowMs);
  void increaseTarget();
  void decreaseTarget();
  ControlStatus status() const;
  unsigned long errorCooldownElapsedMs(unsigned long nowMs) const;

private:
  Settings *settings_ = nullptr;
  ControlStatus status_;
  unsigned long modeStartedMs_ = 0;
  unsigned long lastControlLogMs_ = 0;
  float integralC_ = 0.0f;
  float lastErrorC_ = 0.0f;
  float lastPitC_ = 0.0f;
  float lastDTerm_ = 0.0f;
  unsigned long lastPidMs_ = 0;

  void enterMode(SmokerMode mode, unsigned long nowMs, const char *errorMessage = nullptr);
  void calculatePid(float pitTempC, unsigned long nowMs);
  void updateOutputs(unsigned long nowMs);
  void fail(unsigned long nowMs, const char *message);
};
