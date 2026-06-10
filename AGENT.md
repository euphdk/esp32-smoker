# Agent Notes

## Project Intent

This repository is a proof-of-concept ESP32-S3 CYD pellet smoker controller. Prioritize a working demonstration over production completeness.

## Safety Boundary

- Do not add mains-voltage wiring instructions.
- Treat auger, fan, and igniter as simulated low-voltage GPIO outputs.
- Keep relay/SSR integration comments high level and inside the output abstraction.
- Do not remove safety placeholders even though this is only a POC.

## Architecture

- Keep UI, touch input, sensor handling, control logic, and outputs separated.
- Avoid blocking delays in control paths.
- Use `millis()`-based scheduling unless a future task explicitly introduces FreeRTOS tasks.
- Keep CYD pin assumptions centralized in `include/Config.h` and `platformio.ini`.

## Style

- Keep changes minimal and practical.
- Use Celsius for all temperatures.
- Prefer Serial logs for POC observability.
- Document hardware assumptions in `README.md` when they change.
