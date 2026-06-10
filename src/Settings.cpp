#include "Settings.h"

#include <Arduino.h>

#include "Config.h"

namespace {
constexpr const char *kNamespace = "smoker";
constexpr const char *kKeyTarget = "targetC";
constexpr const char *kKeyCal = "calC";

float clampTarget(float v) {
  if (v < Config::MinTargetC) return Config::MinTargetC;
  if (v > Config::MaxTargetC) return Config::MaxTargetC;
  return v;
}

float clampCal(float v) {
  if (v < Config::CalibrationMinC) return Config::CalibrationMinC;
  if (v > Config::CalibrationMaxC) return Config::CalibrationMaxC;
  return v;
}
} // namespace

void Settings::begin() {
  prefs_.begin(kNamespace, false);
  targetC_ = clampTarget(prefs_.getFloat(kKeyTarget, Config::InitialTargetC));
  calibrationC_ = clampCal(prefs_.getFloat(kKeyCal, 0.0f));
  loaded_ = true;
  Serial.printf("[settings] loaded target=%.1fC cal=%.1fC\n", targetC_, calibrationC_);
}

void Settings::setTargetC(float v) {
  const float clamped = clampTarget(v);
  if (loaded_ && clamped == targetC_) {
    return;
  }
  targetC_ = clamped;
  dirty_ = true;
}

void Settings::setCalibrationC(float v) {
  const float clamped = clampCal(v);
  if (loaded_ && clamped == calibrationC_) {
    return;
  }
  calibrationC_ = clamped;
  dirty_ = true;
}

void Settings::flush(unsigned long nowMs) {
  if (!dirty_) {
    return;
  }
  if (nowMs - lastWriteMs_ < Config::SettingsFlushMs) {
    return;
  }
  prefs_.putFloat(kKeyTarget, targetC_);
  prefs_.putFloat(kKeyCal, calibrationC_);
  dirty_ = false;
  lastWriteMs_ = nowMs;
  Serial.printf("[settings] saved target=%.1fC cal=%.1fC\n", targetC_, calibrationC_);
}
