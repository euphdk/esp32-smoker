#include "Types.h"

const char *modeToString(SmokerMode mode) {
  switch (mode) {
  case SmokerMode::Idle:
    return "Idle";
  case SmokerMode::Startup:
    return "Startup";
  case SmokerMode::Running:
    return "Running";
  case SmokerMode::Shutdown:
    return "Shutdown";
  case SmokerMode::Error:
    return "Error";
  case SmokerMode::ErrorCooldown:
    return "Cooldown";
  }

  return "Unknown";
}
