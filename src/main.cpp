#include <Arduino.h>

#include "App.h"

App app;

static void onMqttTarget(float v) { app.setTarget(v); }
static void onMqttCalibration(float v) { app.setCalibration(v); }
static void onMqttAck(unsigned long nowMs) { app.acknowledgeError(nowMs); }

void setup() {
  app.begin();
  app.setMqttCommandHandlers(onMqttTarget, onMqttCalibration, onMqttAck);
}

void loop() { app.loop(); }
