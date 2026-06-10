#pragma once

#include <ESPAsyncWebServer.h>

#include "Controller.h"
#include "Network.h"
#include "Settings.h"
#include "StatusSnapshot.h"

class WebUi {
public:
  void begin(Settings &settings, Network &network, Controller &controller);
  void loop();
  void updateSnapshot(const StatusSnapshot &snapshot);

private:
  AsyncWebServer server_{80};
  Settings *settings_ = nullptr;
  Network *network_ = nullptr;
  Controller *controller_ = nullptr;
  StatusSnapshot latest_;
  bool haveSnapshot_ = false;
};

