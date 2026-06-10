#include <Arduino.h>

#include "App.h"

App app;

static void onMqttTarget(float v) { app.enqueueTarget(v); }
static void onMqttCalibration(float v) { app.enqueueCalibration(v); }
static void onMqttAck(unsigned long nowMs) { app.enqueueAck(nowMs); }

void setup() {
  app.begin();
  app.setMqttCommandHandlers(onMqttTarget, onMqttCalibration, onMqttAck);
}

void loop() { app.loop(); }
