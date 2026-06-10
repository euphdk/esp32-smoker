#include "Settings.h"

#include <Arduino.h>

#include "Config.h"
#include "Log.h"

namespace {
constexpr const char *kNamespace = "smoker";
constexpr const char *kKeyTarget = "targetC";
constexpr const char *kKeyCal = "calC";
constexpr const char *kKeyWifiSsid = "wifiSsid";
constexpr const char *kKeyWifiPass = "wifiPass";
constexpr const char *kKeyMqttHost = "mqttHost";
constexpr const char *kKeyMqttPort = "mqttPort";
constexpr const char *kKeyMqttUser = "mqttUser";
constexpr const char *kKeyMqttPass = "mqttPass";
constexpr const char *kKeyMqttBase = "mqttBase";

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

String clampString(const String &v, size_t maxLen) {
  if (v.length() <= maxLen) return v;
  return v.substring(0, maxLen);
}
} // namespace

void Settings::begin() {
  prefs_.begin(kNamespace, false);
  targetC_ = clampTarget(prefs_.getFloat(kKeyTarget, Config::InitialTargetC));
  calibrationC_ = clampCal(prefs_.getFloat(kKeyCal, 0.0f));
  wifiSsid_ = prefs_.getString(kKeyWifiSsid, "");
  wifiPass_ = prefs_.getString(kKeyWifiPass, "");
  mqttHost_ = prefs_.getString(kKeyMqttHost, "");
  mqttPort_ = prefs_.getUShort(kKeyMqttPort, 1883);
  mqttUser_ = prefs_.getString(kKeyMqttUser, "");
  mqttPass_ = prefs_.getString(kKeyMqttPass, "");
  mqttBaseTopic_ = clampString(prefs_.getString(kKeyMqttBase, "smoker"), 32);
  loaded_ = true;
  Log.printf("[settings] loaded target=%.1fC cal=%.1fC wifi=%s mqtt=%s:%u base=%s\n",
             targetC_, calibrationC_,
             wifiSsid_.length() ? wifiSsid_.c_str() : "(unset)",
             mqttHost_.length() ? mqttHost_.c_str() : "(unset)",
             mqttPort_, mqttBaseTopic_.c_str());
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

void Settings::setWifiSsid(const String &v) {
  const String clamped = clampString(v, 32);
  if (loaded_ && clamped == wifiSsid_) {
    return;
  }
  wifiSsid_ = clamped;
  dirty_ = true;
  if (onConfigChange_) onConfigChange_();
}

void Settings::setWifiPass(const String &v) {
  const String clamped = clampString(v, 64);
  if (loaded_ && clamped == wifiPass_) {
    return;
  }
  wifiPass_ = clamped;
  dirty_ = true;
  if (onConfigChange_) onConfigChange_();
}

void Settings::setMqttHost(const String &v) {
  const String clamped = clampString(v, 64);
  if (loaded_ && clamped == mqttHost_) {
    return;
  }
  mqttHost_ = clamped;
  dirty_ = true;
  if (onConfigChange_) onConfigChange_();
}

void Settings::setMqttPort(uint16_t v) {
  if (v == 0) v = 1883;
  if (loaded_ && v == mqttPort_) {
    return;
  }
  mqttPort_ = v;
  dirty_ = true;
  if (onConfigChange_) onConfigChange_();
}

void Settings::setMqttUser(const String &v) {
  const String clamped = clampString(v, 32);
  if (loaded_ && clamped == mqttUser_) {
    return;
  }
  mqttUser_ = clamped;
  dirty_ = true;
  if (onConfigChange_) onConfigChange_();
}

void Settings::setMqttPass(const String &v) {
  const String clamped = clampString(v, 64);
  if (loaded_ && clamped == mqttPass_) {
    return;
  }
  mqttPass_ = clamped;
  dirty_ = true;
  if (onConfigChange_) onConfigChange_();
}

void Settings::setMqttBaseTopic(const String &v) {
  const String clamped = clampString(v, 32);
  if (clamped.length() == 0) return;
  if (loaded_ && clamped == mqttBaseTopic_) {
    return;
  }
  mqttBaseTopic_ = clamped;
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
  prefs_.putString(kKeyWifiSsid, wifiSsid_);
  prefs_.putString(kKeyWifiPass, wifiPass_);
  prefs_.putString(kKeyMqttHost, mqttHost_);
  prefs_.putUShort(kKeyMqttPort, mqttPort_);
  prefs_.putString(kKeyMqttUser, mqttUser_);
  prefs_.putString(kKeyMqttPass, mqttPass_);
  prefs_.putString(kKeyMqttBase, mqttBaseTopic_);
  dirty_ = false;
  lastWriteMs_ = nowMs;
  Log.printf("[settings] saved (target=%.1fC cal=%.1fC wifi=%s mqtt=%s:%u base=%s)\n",
             targetC_, calibrationC_,
             wifiSsid_.length() ? wifiSsid_.c_str() : "(unset)",
             mqttHost_.length() ? mqttHost_.c_str() : "(unset)",
             mqttPort_, mqttBaseTopic_.c_str());
}
