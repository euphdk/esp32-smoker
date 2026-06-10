#include "WebUi.h"

#include <ArduinoJson.h>

#include <Arduino.h>

#include "Config.h"
#include "Log.h"

namespace {

const char kIndexHtml[] PROGMEM = R"rawliteral(
<!doctype html>
<html><head>
<meta charset="utf-8">
<title>Pellet Smoker</title>
<meta http-equiv="refresh" content="2">
<style>
body{font-family:sans-serif;max-width:560px;margin:24px auto;padding:0 12px;color:#222;background:#fafafa}
h1{font-size:1.4em;margin:0 0 8px}
.card{border:1px solid #ddd;border-radius:6px;padding:12px 16px;margin:12px 0;background:#fff}
.row{display:flex;justify-content:space-between;padding:2px 0}
.label{color:#666}
.value{font-weight:600}
form{margin:8px 0}
input[type=number]{width:6em;padding:4px}
button{padding:6px 12px;margin-left:8px;cursor:pointer}
.err{color:#a00;font-weight:600}
</style>
</head><body>
<h1>Pellet Smoker</h1>
<div class="card" id="status">
  <div class="row"><span class="label">Pit</span><span class="value" id="pit">%PIT% &deg;C</span></div>
  <div class="row"><span class="label">Target</span><span class="value" id="target">%TARGET% &deg;C</span></div>
  <div class="row"><span class="label">Calibration</span><span class="value" id="cal">%CAL% &deg;C</span></div>
  <div class="row"><span class="label">Mode</span><span class="value" id="mode">%MODE%</span></div>
  <div class="row"><span class="label">Auger</span><span class="value" id="auger">%AUGER%</span></div>
  <div class="row"><span class="label">Fan</span><span class="value" id="fan">%FAN%</span></div>
  <div class="row"><span class="label">Igniter</span><span class="value" id="igniter">%IGNITER%</span></div>
  <div class="row"><span class="label">WiFi</span><span class="value" id="wifi">%WIFI%</span></div>
  <div class="row"><span class="label">RSSI</span><span class="value" id="rssi">%RSSI% dBm</span></div>
  <div class="row"><span class="label">IP</span><span class="value" id="ip">%IP%</span></div>
  <div class="row"><span class="label">Uptime</span><span class="value" id="uptime">%UPTIME% s</span></div>
  %FAULT_ROW%
</div>
<div class="card">
  <form method="POST" action="/set">
    <span class="label">Set target:</span>
    <input type="number" step="0.5" min="%MINT%" max="%MAXT%" name="target" value="%TARGET%">
    <button type="submit">Apply</button>
  </form>
  <form method="POST" action="/cal">
    <span class="label">Set calibration (&deg;C):</span>
    <input type="number" step="0.1" min="%MINCAL%" max="%MAXCAL%" name="cal" value="%CAL%">
    <button type="submit">Apply</button>
  </form>
  <form method="POST" action="/ack">
    <span class="label">Acknowledge fault:</span>
    <button type="submit">Acknowledge &amp; Reset</button>
  </form>
</div>
<p style="font-size:0.8em;color:#888">No authentication. Intended for trusted LANs only.</p>
</body></html>
)rawliteral";

String renderIndex(const StatusSnapshot &s) {
  String out = kIndexHtml;
  out.replace("%PIT%", String(s.pitC, 1));
  out.replace("%TARGET%", String(s.targetC, 1));
  out.replace("%CAL%", String(s.calibrationC, 1));
  out.replace("%MODE%", modeToString(s.mode));
  out.replace("%AUGER%", s.outputs.auger ? "ON" : "OFF");
  out.replace("%FAN%", s.outputs.fan ? "ON" : "OFF");
  out.replace("%IGNITER%", s.outputs.igniter ? "ON" : "OFF");
  out.replace("%WIFI%", s.wifiConnected ? "connected" : "disconnected");
  out.replace("%RSSI%", String(s.rssi));
  out.replace("%IP%", s.ip);
  out.replace("%UPTIME%", String(s.uptimeMs / 1000UL));
  out.replace("%MINT%", String(Config::MinTargetC, 0));
  out.replace("%MAXT%", String(Config::MaxTargetC, 0));
  out.replace("%MINCAL%", String(Config::CalibrationMinC, 1));
  out.replace("%MAXCAL%", String(Config::CalibrationMaxC, 1));
  if (s.errorMessage != nullptr) {
    String row = String("<div class=\"row\"><span class=\"label\">Fault</span><span class=\"err\">") + s.errorMessage + "</span></div>";
    out.replace("%FAULT_ROW%", row);
  } else {
    out.replace("%FAULT_ROW%", "");
  }
  return out;
}

} // namespace

void WebUi::begin() {

  server_.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!haveSnapshot_) {
      request->send(503, "text/plain", "not ready");
      return;
    }
    request->send(200, "text/html", renderIndex(latest_));
  });

  server_.on("/set", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->hasParam("target", true)) {
      request->send(400, "text/plain", "missing target");
      return;
    }
    const String v = request->getParam("target", true)->value();
    char *end = nullptr;
    const float f = strtof(v.c_str(), &end);
    if (end == v.c_str()) {
      request->send(400, "text/plain", "target not a number");
      return;
    }
    if (cmdTarget_) cmdTarget_(f);
    request->redirect("/");
  });

  server_.on("/cal", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (!request->hasParam("cal", true)) {
      request->send(400, "text/plain", "missing cal");
      return;
    }
    const String v = request->getParam("cal", true)->value();
    char *end = nullptr;
    const float f = strtof(v.c_str(), &end);
    if (end == v.c_str()) {
      request->send(400, "text/plain", "cal not a number");
      return;
    }
    if (cmdCalibration_) cmdCalibration_(f);
    request->redirect("/");
  });

  server_.on("/ack", HTTP_POST, [this](AsyncWebServerRequest *request) {
    if (cmdAck_) cmdAck_(millis());
    request->redirect("/");
  });

  server_.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
    if (!haveSnapshot_) {
      request->send(503, "application/json", "{}");
      return;
    }
    JsonDocument doc;
    doc["pitC"] = latest_.pitC;
    doc["targetC"] = latest_.targetC;
    doc["calibrationC"] = latest_.calibrationC;
    doc["mode"] = modeToString(latest_.mode);
    doc["auger"] = latest_.outputs.auger ? "ON" : "OFF";
    doc["fan"] = latest_.outputs.fan ? "ON" : "OFF";
    doc["igniter"] = latest_.outputs.igniter ? "ON" : "OFF";
    doc["wifi"] = latest_.wifiConnected;
    doc["rssi"] = latest_.rssi;
    doc["ip"] = latest_.ip;
    doc["uptimeMs"] = latest_.uptimeMs;
    if (latest_.errorMessage != nullptr) {
      doc["error"] = latest_.errorMessage;
    }
    char buf[512];
    const size_t n = serializeJson(doc, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) {
      request->send(500, "text/plain", "serialization failed");
      return;
    }
    request->send(200, "application/json", buf);
  });

  server_.begin();
  Log.println("[web] listening on port 80");
}

void WebUi::loop() {
  // AsyncWebServer does not require per-loop servicing.
}

void WebUi::updateSnapshot(const StatusSnapshot &snapshot) {
  latest_ = snapshot;
  haveSnapshot_ = true;
}

void WebUi::setCommandHandlers(void (*setTarget)(float), void (*setCalibration)(float), void (*ackError)(unsigned long)) {
  cmdTarget_ = setTarget;
  cmdCalibration_ = setCalibration;
  cmdAck_ = ackError;
}
