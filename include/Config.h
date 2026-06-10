#pragma once

#include <Arduino.h>

namespace Config {

constexpr const char *ProjectName = "ESP32 Pellet POC";
constexpr const char *FirmwareVersion = "0.1.0";

constexpr uint8_t AugerPin = 4;
constexpr uint8_t FanPin = 5;
constexpr uint8_t IgniterPin = 6;

constexpr uint8_t TouchCsPin = 9;
constexpr uint8_t TouchIrqPin = 8;

constexpr uint16_t ScreenWidth = 240;
constexpr uint16_t ScreenHeight = 320;

constexpr uint16_t TouchMinX = 300;
constexpr uint16_t TouchMaxX = 3800;
constexpr uint16_t TouchMinY = 300;
constexpr uint16_t TouchMaxY = 3800;

constexpr float InitialTargetC = 107.0f;
constexpr float MinTargetC = 60.0f;
constexpr float MaxTargetC = 180.0f;
constexpr float TargetStepC = 5.0f;

constexpr float InvalidLowC = -20.0f;
constexpr float InvalidHighC = 350.0f;
constexpr float StartupReachedDeltaC = 15.0f;
constexpr uint32_t StartupTimeoutMs = 15UL * 60UL * 1000UL;
constexpr uint32_t ShutdownFanRunMs = 2UL * 60UL * 1000UL;

constexpr uint32_t UiUpdateMs = 500;
constexpr uint32_t ControlUpdateMs = 1000;
constexpr uint32_t SensorUpdateMs = 1000;
constexpr uint32_t LogUpdateMs = 5000;

constexpr uint32_t AugerCycleMs = 10000;
constexpr float ProportionalGain = 3.0f;
constexpr float MinRunningAugerPercent = 5.0f;
constexpr float MaxAugerPercent = 100.0f;

} // namespace Config
