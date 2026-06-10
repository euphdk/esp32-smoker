#include "Outputs.h"

#include <Arduino.h>

#include "Config.h"
#include "Log.h"

void Outputs::begin() {
  pinMode(Config::AugerPin, OUTPUT);
  pinMode(Config::FanPin, OUTPUT);
  pinMode(Config::IgniterPin, OUTPUT);
  digitalWrite(Config::AugerPin, LOW);
  digitalWrite(Config::FanPin, LOW);
  digitalWrite(Config::IgniterPin, LOW);
  apply(OutputState{});
}

void Outputs::apply(const OutputState &state) {
  writeIfChanged("Auger", Config::AugerPin, state_.auger, state.auger);
  writeIfChanged("Fan", Config::FanPin, state_.fan, state.fan);
  writeIfChanged("Igniter", Config::IgniterPin, state_.igniter, state.igniter);

  state_ = state;
}

OutputState Outputs::state() const { return state_; }

void Outputs::writeIfChanged(const char *name, unsigned char pin, bool previous, bool next) {
  if (previous == next) {
    return;
  }

  // POC only: this is a low-voltage simulated output. Relay/SSR integration belongs here later.
  digitalWrite(pin, next ? HIGH : LOW);
  Log.printf("[outputs] %s %s\n", name, next ? "ON" : "OFF");
}
