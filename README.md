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
- Persistent target temperature and calibration offset stored in NVS via ESP32 `Preferences`. NVS writes are throttled to once every 5 s when dirty.
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

Writes are throttled to once per 5 s when a value has changed. To clear the values back to defaults, erase the `smoker` namespace with `pio run --target erase` followed by a fresh flash, or use the Arduino ESP32 NVS partition tool.

## Serial Commands

Open the serial monitor at 115200 baud. Calibration is the only operator-tunable value without an on-screen control:

- `cal=<float>`: Set the calibration offset in °C (e.g. `cal=1.5`, `cal=-0.5`). Range ±20 °C. Out-of-range or non-numeric values are rejected with a log line.
- Anything else is logged as `unknown command: ...` and ignored.

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
- `src/Settings.*`: NVS-backed persistent settings (target, calibration).
- `src/Controller.*`: State machine and simple control logic.
- `src/Outputs.*`: GPIO output abstraction for simulated auger/fan/igniter.
