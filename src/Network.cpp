#include "Network.h"

#include <Arduino.h>

#include "Config.h"
#include "Log.h"

Network *Network::instance_ = nullptr;

void Network::begin(Settings &settings) {
  settings_ = &settings;
  instance_ = this;
  WiFi.onEvent(onWifiEventStatic);
  if (settings_->wifiSsid().length() == 0) {
    wifiState_ = WifiState::Disabled;
    Log.println("[wifi] disabled (no SSID configured)");
  } else {
    startWifi();
  }
}

void Network::loop() {
  if (wifiState_ == WifiState::Connecting) {
    if (WiFi.status() == WL_CONNECTED) {
      // onWifiEventStatic will flip the state.
    } else if (millis() - lastWifiAttemptMs_ > 30000UL) {
      wifiState_ = WifiState::Failed;
      WiFi.disconnect();
      Log.println("[wifi] connect timeout, will retry");
    }
  } else if (wifiState_ == WifiState::Failed || wifiState_ == WifiState::Disabled) {
    if (settings_ != nullptr && settings_->wifiSsid().length() > 0 &&
        millis() - lastWifiAttemptMs_ >= Config::WifiReconnectBackoffMs) {
      startWifi();
    }
  }
}

int Network::rssi() const {
  if (wifiState_ == WifiState::Connected) {
    return WiFi.RSSI();
  }
  return 0;
}

void Network::ensureWifi() {
  if (settings_ == nullptr) return;
  if (settings_->wifiSsid().length() == 0) {
    if (wifiState_ != WifiState::Disabled) {
      WiFi.disconnect();
      wifiState_ = WifiState::Disabled;
      Log.println("[wifi] disabled (SSID cleared)");
    }
    return;
  }
  if (wifiState_ == WifiState::Disabled) {
    startWifi();
  }
}

void Network::startWifi() {
  wifiState_ = WifiState::Connecting;
  lastWifiAttemptMs_ = millis();
  Log.printf("[wifi] connecting to %s\n", settings_->wifiSsid().c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(settings_->wifiSsid().c_str(), settings_->wifiPass().c_str());
}

void Network::onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  (void)info;
  switch (event) {
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    wifiState_ = WifiState::Connected;
    strncpy(ipBuf_, WiFi.localIP().toString().c_str(), sizeof(ipBuf_) - 1);
    ipBuf_[sizeof(ipBuf_) - 1] = '\0';
    Log.printf("[wifi] connected ip=%s rssi=%d\n", ipBuf_, WiFi.RSSI());
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    if (wifiState_ != WifiState::Failed) {
      Log.println("[wifi] disconnected, will retry");
    }
    wifiState_ = WifiState::Failed;
    break;
  default:
    break;
  }
}

void Network::onWifiEventStatic(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (instance_ != nullptr) {
    instance_->onWifiEvent(event, info);
  }
}
