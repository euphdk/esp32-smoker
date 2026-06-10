#pragma once

#include "Controller.h"
#include "LogBuffer.h"
#include "Network.h"
#include "Outputs.h"
#include "Settings.h"
#include "SimulatedTemperatureSensor.h"
#include "TouchInput.h"
#include "Ui.h"
#include "WebUi.h"

class App {
public:
  void begin();
  void loop();

  void setMqttCommandHandlers(void (*setTarget)(float), void (*setCalibration)(float), void (*ackError)(unsigned long));
  void setWebCommandHandlers(void (*setTarget)(float), void (*setCalibration)(float), void (*ackError)(unsigned long));

  void enqueueTarget(float v);
  void enqueueCalibration(float v);
  void enqueueAck(unsigned long nowMs);
  void triggerNetworkReconnect() { network_.triggerReconnect(); }

private:
  enum class CmdType { None, SetTarget, SetCalibration, AckError };
  struct PendingCmd {
    CmdType type = CmdType::None;
    float value = 0.0f;
    unsigned long ackTime = 0;
  };

  void processCommands();

  LogBuffer logBuffer_;
  Settings settings_;
  Ui ui_;
  TouchInput touch_;
  SimulatedTemperatureSensor sensor_;
  Controller controller_;
  Outputs outputs_;
  Network network_;
  WebUi web_;

  static constexpr size_t kCmdQueueSize = 4;
  PendingCmd cmdQueue_[kCmdQueueSize];
  size_t cmdHead_ = 0;
  size_t cmdTail_ = 0;

  static constexpr size_t kSerialBufSize = 64;
  char serialBuf_[kSerialBufSize];
  size_t serialBufLen_ = 0;

  unsigned long lastSensorMs_ = 0;
  unsigned long lastControlMs_ = 0;
  unsigned long lastUiMs_ = 0;

  void handleTouch(unsigned long nowMs);
  void handleSerial();
};

extern App app;
