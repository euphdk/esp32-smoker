#pragma once

#include <ArduinoJson.h>

#include "Config.h"

class Network;

namespace HomeAssistantDiscovery {

inline void addDevice(JsonObject obj, const String &clientId) {
  JsonObject dev = obj["device"].to<JsonObject>();
  JsonArray ids = dev["identifiers"].to<JsonArray>();
  ids.add(clientId);
  dev["name"] = "Pellet Smoker";
  dev["manufacturer"] = "DIY";
  dev["model"] = Config::ProjectName;
  dev["sw_version"] = Config::FirmwareVersion;
}

inline void addAvailability(JsonObject obj, const String &base, const String &clientId) {
  JsonArray avail = obj["availability"].to<JsonArray>();
  JsonObject a = avail.add<JsonObject>();
  a["topic"] = base + "/" + clientId + "/status";
  a["value_template"] = "{{ 'online' if value_json.wifi else 'offline' }}";
}

inline void climate(JsonObject obj, const String &base, const String &clientId) {
  obj["name"] = "Pellet Smoker";
  obj["unique_id"] = clientId + "_climate";
  obj["mode_state_topic"] = base + "/" + clientId + "/status";
  obj["mode_state_template"] = "{{ 'heat' if value_json.mode in ['Startup','Running'] else 'off' }}";
  JsonArray modes = obj["modes"].to<JsonArray>();
  modes.add("off");
  modes.add("heat");
  obj["temperature_command_topic"] = base + "/" + clientId + "/target/set";
  obj["temperature_state_topic"] = base + "/" + clientId + "/status";
  obj["temperature_state_template"] = "{{ value_json.targetC }}";
  obj["current_temperature_topic"] = base + "/" + clientId + "/status";
  obj["current_temperature_template"] = "{{ value_json.pitC }}";
  obj["min_temp"] = Config::MinTargetC;
  obj["max_temp"] = Config::MaxTargetC;
  obj["temp_step"] = Config::TargetStepC;
  addAvailability(obj, base, clientId);
  addDevice(obj, clientId);
}

inline void sensorNumber(JsonObject obj, const String &base, const String &clientId,
                         const char *name, const char *key, const char *id) {
  obj["name"] = name;
  obj["unique_id"] = String(clientId) + "_" + id;
  obj["state_topic"] = base + "/" + clientId + "/status";
  obj["value_template"] = String("{{ value_json.") + key + " }}";
  obj["unit_of_measurement"] = "°C";
  obj["device_class"] = "temperature";
  obj["state_class"] = "measurement";
  addAvailability(obj, base, clientId);
  addDevice(obj, clientId);
}

inline void sensorNumberCal(JsonObject obj, const String &base, const String &clientId) {
  obj["name"] = "Calibration";
  obj["unique_id"] = String(clientId) + "_cal";
  obj["state_topic"] = base + "/" + clientId + "/status";
  obj["value_template"] = "{{ value_json.calibrationC }}";
  obj["unit_of_measurement"] = "°C";
  obj["state_class"] = "measurement";
  addAvailability(obj, base, clientId);
  addDevice(obj, clientId);
}

inline void sensorString(JsonObject obj, const String &base, const String &clientId,
                         const char *name, const char *key, const char *id) {
  obj["name"] = name;
  obj["unique_id"] = String(clientId) + "_" + id;
  obj["state_topic"] = base + "/" + clientId + "/status";
  obj["value_template"] = String("{{ value_json.") + key + " }}";
  addAvailability(obj, base, clientId);
  addDevice(obj, clientId);
}

inline void binarySensor(JsonObject obj, const String &base, const String &clientId,
                         const char *name, const char *key, const char *id) {
  obj["name"] = name;
  obj["unique_id"] = String(clientId) + "_" + id;
  obj["state_topic"] = base + "/" + clientId + "/status";
  obj["value_template"] = String("{{ 'ON' if value_json.") + key + " == 'ON' else 'OFF' }}";
  obj["payload_on"] = "ON";
  obj["payload_off"] = "OFF";
  addAvailability(obj, base, clientId);
  addDevice(obj, clientId);
}

inline void button(JsonObject obj, const String &base, const String &clientId) {
  obj["name"] = "Acknowledge";
  obj["unique_id"] = String(clientId) + "_ack";
  obj["command_topic"] = base + "/" + clientId + "/ack/set";
  obj["payload_press"] = "PRESS";
  addAvailability(obj, base, clientId);
  addDevice(obj, clientId);
}

inline void numberCal(JsonObject obj, const String &base, const String &clientId) {
  obj["name"] = "Calibration";
  obj["unique_id"] = String(clientId) + "_cal_set";
  obj["state_topic"] = base + "/" + clientId + "/status";
  obj["value_template"] = "{{ value_json.calibrationC }}";
  obj["command_topic"] = base + "/" + clientId + "/calibration/set";
  obj["min"] = Config::CalibrationMinC;
  obj["max"] = Config::CalibrationMaxC;
  obj["step"] = 0.5;
  obj["unit_of_measurement"] = "°C";
  obj["mode"] = "box";
  addAvailability(obj, base, clientId);
  addDevice(obj, clientId);
}

} // namespace HomeAssistantDiscovery
