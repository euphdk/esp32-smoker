#include "LogBuffer.h"

void LogBuffer::begin() {
  Log.setBackend(this);
}

void LogBuffer::lineStart() {
  currentLen_ = 0;
}

void LogBuffer::lineByte(char c) {
  if (c == '\r') {
    return;
  }
  if (currentLen_ + 1 < Config::LogLineMaxLen) {
    current_[currentLen_++] = c;
  }
}

void LogBuffer::lineComplete() {
  if (currentLen_ == 0) {
    return;
  }
  current_[currentLen_] = '\0';
  strncpy(lines_[head_], current_, Config::LogLineMaxLen);
  head_ = (head_ + 1) % Config::LogRingCapacity;
  if (count_ < Config::LogRingCapacity) {
    ++count_;
  } else {
    tail_ = (tail_ + 1) % Config::LogRingCapacity;
  }
  currentLen_ = 0;
}

bool LogBuffer::takeLine(char *out, size_t outSize) {
  if (count_ == 0) {
    return false;
  }
  strncpy(out, lines_[tail_], outSize);
  out[outSize - 1] = '\0';
  tail_ = (tail_ + 1) % Config::LogRingCapacity;
  --count_;
  return true;
}
