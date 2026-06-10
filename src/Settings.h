#pragma once

#include <Preferences.h>

#include <Arduino.h>

class Settings {
public:
  void begin();

  float targetC() const { return targetC_; }
  void setTargetC(float v);

  float calibrationC() const { return calibrationC_; }
  void setCalibrationC(float v);

  const String &wifiSsid() const { return wifiSsid_; }
  void setWifiSsid(const String &v);

  const String &wifiPass() const { return wifiPass_; }
  void setWifiPass(const String &v);

  const String &mqttHost() const { return mqttHost_; }
  void setMqttHost(const String &v);

  uint16_t mqttPort() const { return mqttPort_; }
  void setMqttPort(uint16_t v);

  const String &mqttUser() const { return mqttUser_; }
  void setMqttUser(const String &v);

  const String &mqttPass() const { return mqttPass_; }
  void setMqttPass(const String &v);

  const String &mqttBaseTopic() const { return mqttBaseTopic_; }
  void setMqttBaseTopic(const String &v);

  void flush(unsigned long nowMs);

private:
  Preferences prefs_;
  float targetC_ = 107.0f;
  float calibrationC_ = 0.0f;
  String wifiSsid_;
  String wifiPass_;
  String mqttHost_;
  uint16_t mqttPort_ = 1883;
  String mqttUser_;
  String mqttPass_;
  String mqttBaseTopic_ = "smoker";
  bool dirty_ = false;
  unsigned long lastWriteMs_ = 0;
  bool loaded_ = false;
};
