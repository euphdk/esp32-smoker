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
5. ~~Add persistent settings for target temperature and calibration.~~ Done: new `Settings` module backed by ESP32 `Preferences` (NVS) under namespace `"smoker"`; persists `targetC` (so +/-5C changes survive reboot) and `calibrationC` (an offset in °C applied to the raw sensor reading as `pitC_raw + calC`). Writes are throttled to once every `Config::SettingsFlushMs` (5 s) when dirty; setters clamp incoming values to the configured range and only mark dirty on a real change. `Controller::begin(Settings&)` seeds `status_.targetC` from the persisted value, and the +/-5C setters write through to `Settings` so persistence is automatic. Calibration is editable over Serial with the `cal=<float>` command (range `Config::CalibrationMinC`..`Config::CalibrationMaxC`, i.e. -20..+20 °C). The `[sensor]` log line now includes the calibration offset.
6. ~~Add explicit fault screens and operator acknowledgement flow.~~ Done: new `SmokerMode::ErrorCooldown` state entered via `Controller::acknowledgeError`; full red `Ui::drawError` screen with FAULT header, error message, output state, current pit temp, and a single "Acknowledge & Reset" button; 30 s post-ack fan cooldown (configurable via `Config::PostAckCooldownMs`); Stop is blocked in Error and ErrorCooldown so only the Ack button can clear a fault; every `fail()` call now logs (rate-limited by the existing 5 s log throttle) instead of only on first entry; all other touch input is suppressed in fault.
7. Add production safety review before any real auger, fan, igniter, relay, SSR, or mains-voltage integration.
8. Add WiFi, webserver, and MQTT with Home Assistant support. Done: new `Network` module owns the WiFi station lifecycle (5 s reconnect backoff) and an `AsyncMqttClient` instance (host as IP or hostname, optional credentials, retained status JSON to `<base>/<id>/status` throttled to 1 Hz, log mirror as a JSON array to `<base>/<id>/log` throttled to 100 ms). New `WebUi` module serves an `AsyncWebServer` (port 80) with a single-page status display (auto-refresh every 2 s) and three write endpoints (`/set`, `/cal`, `/ack`). On every MQTT connect, `HomeAssistantDiscovery` publishes retained discovery messages for a `climate` entity (the smoker as a thermostat), `sensor` entities for pit/target/calibration/mode, `binary_sensor` entities for auger/fan/igniter, a `button` for Ack, and a `number` for calibration; command topics (`<base>/<id>/target/set`, `.../calibration/set`, `.../ack/set`) are subscribed and routed through the existing `Controller::setTarget` / `Settings::setCalibrationC` / `Controller::acknowledgeError` write paths. Start/Stop remain touchscreen-only by design. New `LogBuffer` is a 64-line ring buffer fed by the new `MqttSerial` (a `Print` subclass over `Serial`); writes to `Log` are mirrored to the real `Serial` and to the buffer, and `Network::drainLogs` batches the buffer to MQTT. New `StatusSnapshot` is a POD struct built once per `App::loop` tick and consumed by both the network and the web layer so the three views are always sourced from the same state. Credentials are stored in NVS under the same `"smoker"` namespace as the existing settings and are entered over Serial with `wifi_ssid=`, `wifi_pass=`, `mqtt_host=`, `mqtt_port=`, `mqtt_user=`, `mqtt_pass=`, and `mqtt_base=`. The web UI is unauthenticated and intended for trusted LANs only.
