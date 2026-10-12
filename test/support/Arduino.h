#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>

// Minimal Arduino String for native (host) unit tests. Enough surface for the
// firmware logic under test and for ArduinoJson's `.as<const char*>()` path
// (ArduinoJson fills std::string / const char*, not an arbitrary String stub).
class String {
  std::string s;
public:
  String() {}
  String(const char* c) : s(c ? c : "") {}
  String(const std::string& c) : s(c) {}

  const char* c_str() const { return s.c_str(); }
  size_t length() const { return s.size(); }

  bool operator==(const String& o) const { return s == o.s; }
  bool operator==(const char* o) const { return s == (o ? o : ""); }
  bool operator!=(const char* o) const { return !(*this == o); }
  String& operator=(const char* c) { s = c ? c : ""; return *this; }
};

constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int INPUT = 0;
constexpr int OUTPUT = 1;
constexpr int INPUT_PULLUP = 1;
constexpr int INPUT_PULLDOWN = 2;

namespace arduino_test {
inline int read_value = LOW;
inline uint32_t current_millis = 0;
inline int pin_mode_pin = -1;
inline int pin_mode_value = -1;
inline int write_pins[2] = {-1, -1};
inline int write_values[2] = {-1, -1};
inline int write_count = 0;
}

inline void pinMode(int pin, int mode) {
  arduino_test::pin_mode_pin = pin;
  arduino_test::pin_mode_value = mode;
}
inline int digitalRead(int) { return arduino_test::read_value; }
inline void digitalWrite(int pin, int value) {
  if (arduino_test::write_count < 2) {
    arduino_test::write_pins[arduino_test::write_count] = pin;
    arduino_test::write_values[arduino_test::write_count] = value;
  }
  ++arduino_test::write_count;
}
inline uint32_t millis() { return arduino_test::current_millis; }

namespace arduino_test {
inline void reset() {
  read_value = LOW;
  current_millis = 0;
  pin_mode_pin = -1;
  pin_mode_value = -1;
  write_pins[0] = write_pins[1] = -1;
  write_values[0] = write_values[1] = -1;
  write_count = 0;
}
inline void setDigitalRead(int value) { read_value = value; }
inline void setMillis(uint32_t value) { current_millis = value; }
inline int lastPinModePin() { return pin_mode_pin; }
inline int lastPinModeValue() { return pin_mode_value; }
inline int digitalWriteCount() { return write_count; }
inline int digitalWritePin(int index) { return write_pins[index]; }
inline int digitalWriteValue(int index) { return write_values[index]; }
}
