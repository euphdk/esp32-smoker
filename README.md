# ESP32-S3 CYD Pellet Smoker Controller POC

Proof-of-concept pellet smoker controller firmware for an ESP32-S3 CYD / Chinese Yellow Display.

This is not a production-safe appliance controller. It demonstrates a touchscreen UI, simulated pit temperature, basic control logic, and simulated low-voltage GPIO outputs for an auger, combustion fan, and igniter.

## Features

- Boot screen with project name and firmware version.
- Main screen showing current pit temperature, target temperature, output states, and mode.
- Fault screen with a single "Acknowledge & Reset" button, entered automatically from Error. Stop is suppressed while in fault.
- Touch controls for target temperature up/down, start cook, and stop/shutdown.
- Simulated temperature source structured behind a sensor interface.
- Non-blocking `millis()` based scheduling.
- State machine: Idle, Startup, Running, Shutdown, Error, ErrorCooldown.
- PID control (Kp/Ki/Kd with anti-windup) mapped to time-based auger duty cycle.
- Persistent settings in NVS (target temperature, calibration offset, WiFi and MQTT credentials). NVS writes are throttled to once every 5 s when dirty.
- WiFi station connect with 5 s backoff reconnect.
- Web status page (`http://<device-ip>/`) and three write endpoints (set target, set calibration, ack error). No authentication; intended for trusted LANs only.
- MQTT with Home Assistant discovery. A `climate` entity, sensors, binary sensors, an Ack button, and a calibration number are auto-published. Status JSON is published to `<base>/<id>/status` and log lines are mirrored to `<base>/<id>/log`.
- Safety placeholders for invalid sensor readings, startup timeout, and forced operator acknowledgement of faults.
- Serial logging for state changes, temperature updates, control output, and output changes.

## Hardware Assumptions

Target board families sold as ESP32-S3 CYD vary by display controller, touch controller, and pinout. This POC starts with common ILI9341 + XPT2046 assumptions and keeps board pins centralized in `include/Config.h` and `platformio.ini`.

If your display is blank or touch does not respond, verify your exact CYD schematic and update the TFT/touch pins.

## Simulated Output Pins

These pins are intended for low-voltage LEDs or logic-level test signals only:

| Function | GPIO |
| --- | --- |
| Auger | 4 |
| Fan | 5 |
| Igniter | 6 |

Do not connect these directly to mains-voltage equipment. Future relay/SSR integration belongs behind the output abstraction in `src/Outputs.cpp` with proper electrical design outside this POC.

## Touch Controls

On the main screen:

- `+`: Increase target temperature by 5 C.
- `-`: Decrease target temperature by 5 C.
- `Start`: Enter startup mode.
- `Stop`: Enter shutdown mode.

On the fault screen (Error or ErrorCooldown):

- `Acknowledge & Reset`: Clear the fault and start a 30 s fan cooldown. The Stop button is suppressed in fault; only this button can clear it.

Temperatures are displayed in Celsius.

## Persistent Settings

Stored in the ESP32 NVS under namespace `smoker`:

| Key | Type | Default | Notes |
| --- | --- | --- | --- |
| `targetC` | float | `Config::InitialTargetC` (107) | Last operator-set target. Updated automatically by the +/- buttons. Clamped to `Config::MinTargetC`..`Config::MaxTargetC`. |
| `calC` | float | 0.0 | Calibration offset in °C, applied as `pitC_raw + calC` before display and control. Clamped to `Config::CalibrationMinC`..`Config::CalibrationMaxC` (±20). |
| `wifiSsid` | string | empty | WiFi SSID. Empty disables WiFi. |
| `wifiPass` | string | empty | WiFi password. Max 64 chars. |
| `mqttHost` | string | empty | MQTT broker host (IP or hostname). Empty disables MQTT. |
| `mqttPort` | uint16 | 1883 | MQTT broker port. |
| `mqttUser` | string | empty | Optional MQTT username. Empty disables credentials (anonymous). |
| `mqttPass` | string | empty | Optional MQTT password. Max 64 chars. |
| `mqttBase` | string | `smoker` | Base topic prefix. The client id (`<base>/<id>/...`) is appended. |

Writes are throttled to once per 5 s when a value has changed. To clear the values back to defaults, erase the `smoker` namespace with `pio run --target erase` followed by a fresh flash, or use the Arduino ESP32 NVS partition tool.

## Serial Commands

Open the serial monitor at 115200 baud. All commands take effect on the next 5 s flush and survive reboot.

- `cal=<float>`: Set the calibration offset in °C (e.g. `cal=1.5`, `cal=-0.5`). Range ±20 °C.
- `wifi_ssid=<text>`: Set the WiFi SSID. Max 32 chars.
- `wifi_pass=<text>`: Set the WiFi password. Max 64 chars. Echoed as `***`.
- `mqtt_host=<text>`: Set the MQTT broker host (IP or hostname). Max 64 chars.
- `mqtt_port=<1..65535>`: Set the MQTT broker port. `0` is coerced to 1883.
- `mqtt_user=<text>`: Set the MQTT username. Empty disables credentials.
- `mqtt_pass=<text>`: Set the MQTT password. Echoed as `***`.
- `mqtt_base=<text>`: Set the base topic prefix. Default `smoker`. Max 32 chars. Reject empty.

Out-of-range or non-numeric values are rejected with a log line. Anything else is logged as `unknown command: ...` and ignored.

## Web Interface

Once WiFi is connected, the IP address is logged on Serial as `ip=<addr>`. Open `http://<ip>/` in a browser for a single-page status display that auto-refreshes every 2 seconds. The page shows pit/target/calibration, mode, output states, WiFi state, RSSI, IP, uptime, and the current fault message if any.

Three POST forms on the page let you:

- Set the target temperature (`/set`).
- Set the calibration offset (`/cal`).
- Acknowledge a fault (`/ack`).

A `GET /api/status` endpoint returns the same data as JSON. **There is no authentication.** The web interface is intended for trusted LANs only — anyone on the same network can change the target and the calibration. Start/Stop remain touchscreen-only by design and are not exposed to the web or MQTT.

## MQTT and Home Assistant

When an MQTT host is configured, the device connects after WiFi is up and uses a client id of the form `smoker-XXXXXX` derived from the MAC address. On every connect it publishes Home Assistant discovery messages under `homeassistant/<component>/<id>/config` and subscribes to its own command topics. The full set of entities:

- `climate` (the smoker as a thermostat) with current/target temperature, mode `off`/`heat`.
- `sensor` for pit, target, calibration, and mode.
- `binary_sensor` for auger, fan, igniter.
- `button` for "Acknowledge & Reset".
- `number` for calibration (writes round-trip to the same MQTT path).

All entities share a `device` block (`identifiers: ["<clientId>"]`, `name: "Pellet Smoker"`) and use `<base>/<id>/status` for availability.

Status is published as a retained JSON object to `<base>/<id>/status` at most once per second. Log lines are batched into a JSON array and published to `<base>/<id>/log` at most every 100 ms.

Inbound command topics:

- `<base>/<id>/target/set` (float): sets the target temperature.
- `<base>/<id>/calibration/set` (float): sets the calibration offset.
- `<base>/<id>/ack/set` (payload `PRESS`): acknowledges the active fault.

Start/Stop are deliberately not exposed.

## Build And Upload

Install PlatformIO, then run:

```sh
pio run
pio run --target upload
pio device monitor
```

The default environment is `esp32-s3-cyd`.

## Project Structure

- `include/Config.h`: Firmware version, pin map, timing, UI, and control constants.
- `src/App.*`: Top-level application orchestration.
- `src/Ui.*`: TFT drawing and touch target layout.
- `src/TouchInput.*`: Touchscreen polling and calibration mapping.
- `src/TemperatureSensor.h`: Sensor interface.
- `src/SimulatedTemperatureSensor.*`: Simulated pit temperature model.
- `src/Settings.*`: NVS-backed persistent settings (target, calibration, WiFi and MQTT credentials).
- `src/Controller.*`: State machine and PID control.
- `src/Outputs.*`: GPIO output abstraction for simulated auger/fan/igniter.
- `src/Log.{h,cpp}`: `MqttSerial` (Print subclass over `Serial`).
- `src/LogBuffer.{h,cpp}`: Bounded ring buffer of recent log lines, fed by the `Log` backend.
- `src/Network.{h,cpp}`: WiFi station connect/reconnect and MQTT client (status, log drain, command routing).
- `src/WebUi.{h,cpp}`: Async webserver with status page and write endpoints.
- `src/HomeAssistantDiscovery.h`: JSON discovery payload builders.
- `src/StatusSnapshot.h`: POD struct for outbound state.
