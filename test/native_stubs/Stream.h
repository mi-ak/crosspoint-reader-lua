#pragma once
#include <cstddef>

// Minimal Arduino Stream stub for native builds.
// Provides the read()/readBytes() interface required by ArduinoJson's
// default Reader template when ARDUINOJSON_ENABLE_ARDUINO_STREAM is off.
class Stream {
 public:
  virtual ~Stream() = default;
  virtual int read() { return -1; }
  virtual size_t readBytes(char* /*buffer*/, size_t /*length*/) { return 0; }
};
