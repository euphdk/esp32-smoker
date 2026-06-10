#pragma once

#include "Types.h"

struct StatusSnapshot {
  float pitC = 0.0f;
  float targetC = 0.0f;
  float calibrationC = 0.0f;
  SmokerMode mode = SmokerMode::Idle;
  OutputState outputs;
  bool wifiConnected = false;
  int rssi = 0;
  const char *ip = "";
  const char *errorMessage = nullptr;
  unsigned long uptimeMs = 0;
};
