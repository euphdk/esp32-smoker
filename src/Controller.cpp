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
    calculatePid(pitTempC, nowMs);
    status_.outputs.fan = true;
    status_.outputs.igniter = true;
    if (pitTempC >= status_.targetC - Config::StartupReachedDeltaC) {
      enterMode(SmokerMode::Running, nowMs);
    } else if (nowMs - modeStartedMs_ > Config::StartupTimeoutMs) {
      fail(nowMs, "Startup temperature timeout");
    }
    break;
  case SmokerMode::Running:
    calculatePid(pitTempC, nowMs);
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
    const float pTerm = Config::PidKp * (status_.targetC - pitTempC);
    Serial.printf("[control] mode=%s pit=%.1fC target=%.1fC output=%.1f%% p=%.1f i=%.1f d=%.1f auger=%d fan=%d igniter=%d\n",
                  modeToString(status_.mode), pitTempC, status_.targetC, status_.controlPercent,
                  pTerm, integralC_, lastDTerm_,
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
  integralC_ = 0.0f;
  lastErrorC_ = 0.0f;
  lastPitC_ = 0.0f;
  lastDTerm_ = 0.0f;
  lastPidMs_ = 0;
}

void Controller::calculatePid(float pitTempC, unsigned long nowMs) {
  if (lastPidMs_ == 0) {
    lastPitC_ = pitTempC;
    lastPidMs_ = nowMs;
    return;
  }

  float dt = static_cast<float>(nowMs - lastPidMs_) / 1000.0f;
  if (dt <= 0.0f) {
    return;
  }

  const float errorC = status_.targetC - pitTempC;
  const float pTerm = Config::PidKp * errorC;

  integralC_ += Config::PidKi * errorC * dt;
  integralC_ = constrain(integralC_, -Config::PidKiMax, Config::PidKiMax);

  const float dTerm = -Config::PidKd * (pitTempC - lastPitC_) / dt;
  float output = pTerm + integralC_ + dTerm;

  if (output > Config::MaxAugerPercent) {
    output = Config::MaxAugerPercent;
    integralC_ -= Config::PidKi * errorC * dt;
    integralC_ = constrain(integralC_, -Config::PidKiMax, Config::PidKiMax);
  } else if (output < 0.0f) {
    output = 0.0f;
    integralC_ -= Config::PidKi * errorC * dt;
    integralC_ = constrain(integralC_, -Config::PidKiMax, Config::PidKiMax);
  }

  if (status_.mode == SmokerMode::Running && output > 0.0f) {
    output = max(output, Config::MinRunningAugerPercent);
  }

  status_.controlPercent = output;
  lastErrorC_ = errorC;
  lastPitC_ = pitTempC;
  lastDTerm_ = dTerm;
  lastPidMs_ = nowMs;
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
