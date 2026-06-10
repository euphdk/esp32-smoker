#pragma once

enum class SmokerMode {
  Idle,
  Startup,
  Running,
  Shutdown,
  Error,
};

struct OutputState {
  bool auger = false;
  bool fan = false;
  bool igniter = false;
};

struct ControlStatus {
  SmokerMode mode = SmokerMode::Idle;
  float targetC = 107.0f;
  float controlPercent = 0.0f;
  OutputState outputs;
  const char *errorMessage = nullptr;
};

const char *modeToString(SmokerMode mode);
