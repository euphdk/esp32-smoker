# Plan

## Current POC

1. Create a PlatformIO Arduino project for ESP32-S3 CYD.
2. Bring up TFT_eSPI display with a boot screen and main status screen.
3. Add XPT2046 touch polling for target up/down, start, and stop controls.
4. Implement simulated pit temperature behind a sensor interface.
5. Implement a basic pellet smoker state machine.
6. Simulate auger, fan, and igniter with GPIO outputs and Serial logs.
7. Keep timing non-blocking using `millis()` scheduling.
8. Document all pin assumptions and safety boundaries.

## Next Steps

1. Confirm the exact ESP32-S3 CYD display and touch pinout.
2. Add board-specific display setup if the default ILI9341/XPT2046 assumptions are wrong.
3. Add a real temperature sensor implementation, likely MAX31865/PT100, MAX31855/MAX6675 thermocouple, or analog thermistor depending on hardware.
4. ~~Replace simple proportional control with a tunable PID implementation.~~ Done: `Controller::calculatePid` with Kp/Ki/Kd and `PidKiMax` anti-windup clamp; derivative on `-dPit/dt` to avoid setpoint kick; PID state reset on every mode transition; P/I/D terms logged on the `[control]` line. Constants live in `include/Config.h` and must be tuned on real hardware.
5. Add persistent settings for target temperature and calibration.
6. ~~Add explicit fault screens and operator acknowledgement flow.~~ Done: new `SmokerMode::ErrorCooldown` state entered via `Controller::acknowledgeError`; full red `Ui::drawError` screen with FAULT header, error message, output state, current pit temp, and a single "Acknowledge & Reset" button; 30 s post-ack fan cooldown (configurable via `Config::PostAckCooldownMs`); Stop is blocked in Error and ErrorCooldown so only the Ack button can clear a fault; every `fail()` call now logs (rate-limited by the existing 5 s log throttle) instead of only on first entry; all other touch input is suppressed in fault.
7. Add production safety review before any real auger, fan, igniter, relay, SSR, or mains-voltage integration.
