#pragma once

#include <AsyncMqttClient.h>
#include <WiFi.h>

#include "LogBuffer.h"
#include "Settings.h"
#include "StatusSnapshot.h"

enum class WifiState {
  Disabled,
  Connecting,
  Connected,
  Failed,
};

enum class MqttState {
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

  MqttState mqttState() const { return mqttState_; }
  bool mqttConnected() const { return mqttState_ == MqttState::Connected; }

  void publishStatus(const StatusSnapshot &snapshot);
  void drainLogs(LogBuffer &logBuffer);

  const String &clientId() const { return clientId_; }
  const String &baseTopic() const;

private:
  void ensureWifi();
  void startWifi();
  void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info);
  static void onWifiEventStatic(WiFiEvent_t event, WiFiEventInfo_t info);

  void ensureMqtt();
  void startMqtt();
  void onMqttConnect(bool sessionPresent);
  void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
  void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
  static void onMqttConnectStatic(bool sessionPresent);
  static void onMqttDisconnectStatic(AsyncMqttClientDisconnectReason reason);
  static void onMqttMessageStatic(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);

  Settings *settings_ = nullptr;
  AsyncMqttClient mqtt_;
  String clientId_;
  WifiState wifiState_ = WifiState::Disabled;
  MqttState mqttState_ = MqttState::Disabled;
  unsigned long lastWifiAttemptMs_ = 0;
  unsigned long lastMqttAttemptMs_ = 0;
  unsigned long lastStatusPublishMs_ = 0;
  unsigned long lastLogDrainMs_ = 0;
  bool statusDirty_ = false;
  char ipBuf_[16] = "0.0.0.0";
  static Network *instance_;
};
