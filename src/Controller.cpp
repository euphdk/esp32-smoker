#include "Controller.h"

#include <Arduino.h>

#include "Config.h"

void Controller::begin() {
  status_.targetC = Config::InitialTargetC;
  enterMode(SmokerMode::Idle, millis());
}

void Controller::update(unsigned long nowMs, float pitTempC, bool sensorValid) {
  if (!sensorValid || pitTempC < Config::InvalidLowC || pitTempC > Config::InvalidHighC) {
    fail(nowMs, "Invalid sensor value");
  }

  switch (status_.mode) {
  case SmokerMode::Idle:
    status_.controlPercent = 0.0f;
    status_.outputs = OutputState{};
    break;
  case SmokerMode::Startup:
    calculateControl(pitTempC);
    status_.outputs.fan = true;
    status_.outputs.igniter = true;
    if (pitTempC >= status_.targetC - Config::StartupReachedDeltaC) {
      enterMode(SmokerMode::Running, nowMs);
    } else if (nowMs - modeStartedMs_ > Config::StartupTimeoutMs) {
      fail(nowMs, "Startup temperature timeout");
    }
    break;
  case SmokerMode::Running:
    calculateControl(pitTempC);
    status_.outputs.fan = true;
    status_.outputs.igniter = false;
    break;
  case SmokerMode::Shutdown:
    status_.controlPercent = 0.0f;
    status_.outputs.auger = false;
    status_.outputs.igniter = false;
    status_.outputs.fan = nowMs - modeStartedMs_ < Config::ShutdownFanRunMs;
    if (!status_.outputs.fan) {
      enterMode(SmokerMode::Idle, nowMs);
    }
    break;
  case SmokerMode::Error:
    status_.controlPercent = 0.0f;
    status_.outputs.auger = false;
    status_.outputs.igniter = false;
    status_.outputs.fan = true;
    break;
  }

  updateOutputs(nowMs);

  if (nowMs - lastControlLogMs_ >= Config::LogUpdateMs) {
    lastControlLogMs_ = nowMs;
    Serial.printf("[control] mode=%s pit=%.1fC target=%.1fC output=%.1f%% auger=%d fan=%d igniter=%d\n",
                  modeToString(status_.mode), pitTempC, status_.targetC, status_.controlPercent,
                  status_.outputs.auger, status_.outputs.fan, status_.outputs.igniter);
  }
}

void Controller::start(unsigned long nowMs) {
  if (status_.mode == SmokerMode::Idle || status_.mode == SmokerMode::Shutdown) {
    enterMode(SmokerMode::Startup, nowMs);
  }
}

void Controller::stop(unsigned long nowMs) {
  if (status_.mode != SmokerMode::Idle) {
    enterMode(SmokerMode::Shutdown, nowMs);
  }
}

void Controller::increaseTarget() {
  status_.targetC = min(Config::MaxTargetC, status_.targetC + Config::TargetStepC);
  Serial.printf("[control] target=%.1fC\n", status_.targetC);
}

void Controller::decreaseTarget() {
  status_.targetC = max(Config::MinTargetC, status_.targetC - Config::TargetStepC);
  Serial.printf("[control] target=%.1fC\n", status_.targetC);
}

ControlStatus Controller::status() const { return status_; }

void Controller::enterMode(SmokerMode mode, unsigned long nowMs, const char *errorMessage) {
  if (status_.mode != mode) {
    Serial.printf("[state] %s -> %s\n", modeToString(status_.mode), modeToString(mode));
  }

  status_.mode = mode;
  status_.errorMessage = errorMessage;
  modeStartedMs_ = nowMs;
}

void Controller::calculateControl(float pitTempC) {
  const float errorC = status_.targetC - pitTempC;
  float output = errorC * Config::ProportionalGain;

  if (status_.mode == SmokerMode::Running && output > 0.0f) {
    output = max(output, Config::MinRunningAugerPercent);
  }

  status_.controlPercent = constrain(output, 0.0f, Config::MaxAugerPercent);
}

void Controller::updateOutputs(unsigned long nowMs) {
  if (status_.mode != SmokerMode::Startup && status_.mode != SmokerMode::Running) {
    return;
  }

  const uint32_t cyclePosition = nowMs % Config::AugerCycleMs;
  const uint32_t onTime = static_cast<uint32_t>(Config::AugerCycleMs * (status_.controlPercent / 100.0f));
  status_.outputs.auger = cyclePosition < onTime;
}

void Controller::fail(unsigned long nowMs, const char *message) {
  if (status_.mode != SmokerMode::Error) {
    Serial.printf("[error] %s\n", message);
    enterMode(SmokerMode::Error, nowMs, message);
  }
}
