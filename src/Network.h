#pragma once

#include <WiFi.h>

#include "Settings.h"

enum class WifiState {
  Disabled,
  Connecting,
  Connected,
  Failed,
};

class Network {
public:
  void begin(Settings &settings);
  void loop();

  WifiState wifiState() const { return wifiState_; }
  bool wifiConnected() const { return wifiState_ == WifiState::Connected; }
  int rssi() const;
  const char *ipAddress() const { return ipBuf_; }

private:
  void ensureWifi();
  void startWifi();
  void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
  static void onWifiEventStatic(WiFiEvent_t event, WiFiEventInfo_t info);

  Settings *settings_ = nullptr;
  WifiState wifiState_ = WifiState::Disabled;
  unsigned long lastWifiAttemptMs_ = 0;
  char ipBuf_[16] = "0.0.0.0";
  static Network *instance_;
};
