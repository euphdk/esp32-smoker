#pragma once

#include "Types.h"

class Outputs {
public:
  void begin();
  void apply(const OutputState &state);
  OutputState state() const;

private:
  OutputState state_;
  void writeIfChanged(const char *name, unsigned char pin, bool previous, bool next);
};
