#pragma once

#include <ESPAsyncWebServer.h>

#include "StatusSnapshot.h"

class WebUi {
public:
  void begin();
  void loop();
  void updateSnapshot(const StatusSnapshot &snapshot);

  void setCommandHandlers(void (*setTarget)(float), void (*setCalibration)(float), void (*ackError)(unsigned long));

private:
  AsyncWebServer server_{80};
  StatusSnapshot latest_;
  bool haveSnapshot_ = false;
  void (*cmdTarget_)(float) = nullptr;
  void (*cmdCalibration_)(float) = nullptr;
  void (*cmdAck_)(unsigned long) = nullptr;
};

