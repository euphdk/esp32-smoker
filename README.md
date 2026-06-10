# ESP32-S3 CYD Pellet Smoker Controller POC

Proof-of-concept pellet smoker controller firmware for an ESP32-S3 CYD / Chinese Yellow Display.

This is not a production-safe appliance controller. It demonstrates a touchscreen UI, simulated pit temperature, basic control logic, and simulated low-voltage GPIO outputs for an auger, combustion fan, and igniter.

## Features

- Boot screen with project name and firmware version.
- Main screen showing current pit temperature, target temperature, output states, and mode.
- Touch controls for target temperature up/down, start cook, and stop/shutdown.
- Simulated temperature source structured behind a sensor interface.
- Non-blocking `millis()` based scheduling.
- Basic state machine: Idle, Startup, Running, Shutdown, Error.
- Simple proportional control mapped to time-based auger duty cycle.
- Safety placeholders for invalid sensor readings and startup timeout.
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

- `+`: Increase target temperature by 5 C.
- `-`: Decrease target temperature by 5 C.
- `Start`: Enter startup mode.
- `Stop`: Enter shutdown mode.

Temperatures are displayed in Celsius.

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
- `src/Controller.*`: State machine and simple control logic.
- `src/Outputs.*`: GPIO output abstraction for simulated auger/fan/igniter.
