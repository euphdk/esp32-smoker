#include "Network.h"

#include <ArduinoJson.h>

#include <Arduino.h>

#include "Config.h"
#include "HomeAssistantDiscovery.h"
#include "Log.h"

Network *Network::instance_ = nullptr;

namespace {
String macSuffix() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[16];
  snprintf(buf, sizeof(buf), "%02x%02x%02x%02x", mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}
} // namespace

void Network::begin(Settings &settings) {
  settings_ = &settings;
  instance_ = this;
  clientId_ = String("smoker-") + macSuffix();
  WiFi.onEvent(onWifiEventStatic);

  mqtt_.onConnect(onMqttConnectStatic);
  mqtt_.onDisconnect(onMqttDisconnectStatic);
  mqtt_.setClientId(clientId_.c_str());

  if (settings_->wifiSsid().length() == 0) {
    wifiState_ = WifiState::Disabled;
    Log.println("[wifi] disabled (no SSID configured)");
  } else {
    startWifi();
  }

  if (settings_->mqttHost().length() == 0) {
    mqttState_ = MqttState::Disabled;
    Log.println("[mqtt] disabled (no host configured)");
  }
}

void Network::loop() {
  ensureWifi();
  ensureMqtt();

  if (statusDirty_ && mqttState_ == MqttState::Connected) {
    if (millis() - lastStatusPublishMs_ >= Config::StatusPublishMinIntervalMs) {
      // Drain will be re-called with new state from App; we just need a ticker.
    }
  }
}

int Network::rssi() const {
  if (wifiState_ == WifiState::Connected) {
    return WiFi.RSSI();
  }
  return 0;
}

const String &Network::baseTopic() const {
  return settings_ != nullptr ? settings_->mqttBaseTopic() : String("smoker");
}

void Network::setCommandTarget(void (*fn)(float)) { cmdTarget_ = fn; }
void Network::setCommandCalibration(void (*fn)(float)) { cmdCalibration_ = fn; }
void Network::setCommandAck(void (*fn)(unsigned long)) { cmdAck_ = fn; }

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
    if (settings_ != nullptr && settings_->mqttHost().length() > 0) {
      if (mqttState_ == MqttState::Disabled || mqttState_ == MqttState::Failed) {
        startMqtt();
      }
    }
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    if (wifiState_ != WifiState::Failed) {
      Log.println("[wifi] disconnected, will retry");
    }
    wifiState_ = WifiState::Failed;
    if (mqttState_ == MqttState::Connected) {
      mqtt_.disconnect();
    }
    mqttState_ = MqttState::Failed;
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

void Network::ensureMqtt() {
  if (settings_ == nullptr) return;
  if (settings_->mqttHost().length() == 0) {
    if (mqttState_ != MqttState::Disabled) {
      mqtt_.disconnect();
      mqttState_ = MqttState::Disabled;
    }
    return;
  }
  if (!wifiConnected()) {
    return;
  }
  if (mqttState_ == MqttState::Disabled || mqttState_ == MqttState::Failed) {
    if (millis() - lastMqttAttemptMs_ >= Config::MqttReconnectBackoffMs) {
      startMqtt();
    }
  }
}

void Network::startMqtt() {
  mqttState_ = MqttState::Connecting;
  lastMqttAttemptMs_ = millis();
  const uint16_t port = settings_->mqttPort();
  IPAddress ip;
  if (ip.fromString(settings_->mqttHost().c_str())) {
    mqtt_.setServer(ip, port);
  } else {
    mqtt_.setServer(settings_->mqttHost().c_str(), port);
  }
  if (settings_->mqttUser().length() > 0) {
    mqtt_.setCredentials(settings_->mqttUser().c_str(), settings_->mqttPass().c_str());
  } else {
    mqtt_.setCredentials("", "");
  }
  Log.printf("[mqtt] connecting to %s:%u as %s\n",
             settings_->mqttHost().c_str(), port, clientId_.c_str());
  mqtt_.connect();
}

void Network::onMqttConnect(bool sessionPresent) {
  (void)sessionPresent;
  mqttState_ = MqttState::Connected;
  Log.println("[mqtt] connected");

  const String base = baseTopic();
  const String id = clientId_;

  // Subscribe to command topics.
  mqtt_.subscribe((base + "/" + id + "/target/set").c_str(), 0);
  mqtt_.subscribe((base + "/" + id + "/ack/set").c_str(), 0);
  mqtt_.subscribe((base + "/" + id + "/calibration/set").c_str(), 0);

  // Publish Home Assistant discovery.
  auto publishDiscovery = [&](const char *component, const char *objectId, JsonObject obj) {
    char buf[1024];
    const size_t n = serializeJson(obj, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) return;
    const String topic = String(Config::MqttDiscoveryPrefix) + "/" + component + "/" + id + "/" + objectId + "/config";
    mqtt_.publish(topic.c_str(), 0, true, buf, n);
  };

  {
    JsonDocument doc;
    HomeAssistantDiscovery::climate(doc.to<JsonObject>(), base, id);
    publishDiscovery("climate", "climate", doc.as<JsonObject>());
  }
  {
    JsonDocument doc;
    HomeAssistantDiscovery::sensorNumber(doc.to<JsonObject>(), base, id, "Pit Temperature", "pitC", "pit");
    publishDiscovery("sensor", "pit", doc.as<JsonObject>());
  }
  {
    JsonDocument doc;
    HomeAssistantDiscovery::sensorNumber(doc.to<JsonObject>(), base, id, "Target Temperature", "targetC", "target");
    publishDiscovery("sensor", "target", doc.as<JsonObject>());
  }
  {
    JsonDocument doc;
    HomeAssistantDiscovery::sensorNumberCal(doc.to<JsonObject>(), base, id);
    publishDiscovery("sensor", "cal", doc.as<JsonObject>());
  }
  {
    JsonDocument doc;
    HomeAssistantDiscovery::sensorString(doc.to<JsonObject>(), base, id, "Mode", "mode", "mode");
    publishDiscovery("sensor", "mode", doc.as<JsonObject>());
  }
  {
    JsonDocument doc;
    HomeAssistantDiscovery::binarySensor(doc.to<JsonObject>(), base, id, "Auger", "auger", "auger");
    publishDiscovery("binary_sensor", "auger", doc.as<JsonObject>());
  }
  {
    JsonDocument doc;
    HomeAssistantDiscovery::binarySensor(doc.to<JsonObject>(), base, id, "Fan", "fan", "fan");
    publishDiscovery("binary_sensor", "fan", doc.as<JsonObject>());
  }
  {
    JsonDocument doc;
    HomeAssistantDiscovery::binarySensor(doc.to<JsonObject>(), base, id, "Igniter", "igniter", "igniter");
    publishDiscovery("binary_sensor", "igniter", doc.as<JsonObject>());
  }
  {
    JsonDocument doc;
    HomeAssistantDiscovery::button(doc.to<JsonObject>(), base, id);
    publishDiscovery("button", "ack", doc.as<JsonObject>());
  }
  {
    JsonDocument doc;
    HomeAssistantDiscovery::numberCal(doc.to<JsonObject>(), base, id);
    publishDiscovery("number", "cal_set", doc.as<JsonObject>());
  }

  statusDirty_ = true;
}

void Network::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  mqttState_ = MqttState::Failed;
  Log.printf("[mqtt] disconnected reason=%d, will retry\n", static_cast<int>(reason));
}

void Network::onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
                            size_t len, size_t index, size_t total) {
  (void)properties;
  (void)index;
  (void)total;
  if (topic == nullptr || payload == nullptr) return;
  String t = topic;
  String p;
  p.reserve(len + 1);
  for (size_t i = 0; i < len; ++i) {
    p += static_cast<char>(payload[i]);
  }
  p.trim();
  Log.printf("[mqtt] msg topic=%s payload=%s\n", t.c_str(), p.c_str());

  const String base = baseTopic();
  const String id = clientId_;

  if (t == base + "/" + id + "/target/set") {
    if (cmdTarget_ == nullptr) return;
    char *end = nullptr;
    const float v = strtof(p.c_str(), &end);
    if (end == p.c_str()) {
      Log.println("[mqtt] target/set: not a number");
      return;
    }
    cmdTarget_(v);
    return;
  }

  if (t == base + "/" + id + "/calibration/set") {
    if (cmdCalibration_ == nullptr) return;
    char *end = nullptr;
    const float v = strtof(p.c_str(), &end);
    if (end == p.c_str()) {
      Log.println("[mqtt] calibration/set: not a number");
      return;
    }
    cmdCalibration_(v);
    return;
  }

  if (t == base + "/" + id + "/ack/set") {
    if (cmdAck_ == nullptr) return;
    if (p == "PRESS") {
      cmdAck_(millis());
    } else {
      Log.printf("[mqtt] ack/set: unknown payload '%s'\n", p.c_str());
    }
    return;
  }
}

void Network::onMqttConnectStatic(bool sessionPresent) {
  if (instance_ != nullptr) instance_->onMqttConnect(sessionPresent);
}

void Network::onMqttDisconnectStatic(AsyncMqttClientDisconnectReason reason) {
  if (instance_ != nullptr) instance_->onMqttDisconnect(reason);
}

void Network::onMqttMessageStatic(char *topic, char *payload, AsyncMqttClientMessageProperties properties,
                                  size_t len, size_t index, size_t total) {
  if (instance_ != nullptr) instance_->onMqttMessage(topic, payload, properties, len, index, total);
}

void Network::publishStatus(const StatusSnapshot &snapshot) {
  if (mqttState_ != MqttState::Connected) {
    statusDirty_ = true;
    return;
  }
  if (millis() - lastStatusPublishMs_ < Config::StatusPublishMinIntervalMs) {
    statusDirty_ = true;
    return;
  }

  JsonDocument doc;
  doc["pitC"] = snapshot.pitC;
  doc["targetC"] = snapshot.targetC;
  doc["calibrationC"] = snapshot.calibrationC;
  doc["mode"] = modeToString(snapshot.mode);
  doc["auger"] = snapshot.outputs.auger ? "ON" : "OFF";
  doc["fan"] = snapshot.outputs.fan ? "ON" : "OFF";
  doc["igniter"] = snapshot.outputs.igniter ? "ON" : "OFF";
  doc["wifi"] = snapshot.wifiConnected;
  doc["rssi"] = snapshot.rssi;
  doc["ip"] = snapshot.ip;
  doc["uptimeMs"] = snapshot.uptimeMs;
  if (snapshot.errorMessage != nullptr) {
    doc["error"] = snapshot.errorMessage;
  }

  char buf[512];
  const size_t n = serializeJson(doc, buf, sizeof(buf));
  if (n == 0 || n >= sizeof(buf)) return;

  const String topic = baseTopic() + "/" + clientId_ + "/status";
  mqtt_.publish(topic.c_str(), 0, true, buf, n);

  statusDirty_ = false;
  lastStatusPublishMs_ = millis();
}

void Network::drainLogs(LogBuffer &logBuffer) {
  if (mqttState_ != MqttState::Connected) {
    return;
  }
  if (millis() - lastLogDrainMs_ < Config::LogBatchIntervalMs) {
    return;
  }
  if (logBuffer.pending() == 0) {
    lastLogDrainMs_ = millis();
    return;
  }

  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  char line[Config::LogLineMaxLen];
  while (logBuffer.takeLine(line, sizeof(line))) {
    arr.add(line);
  }

  char buf[2048];
  const size_t n = serializeJson(arr, buf, sizeof(buf));
  if (n == 0 || n >= sizeof(buf)) return;

  const String topic = baseTopic() + "/" + clientId_ + "/log";
  mqtt_.publish(topic.c_str(), 0, false, buf, n);
  lastLogDrainMs_ = millis();
}
