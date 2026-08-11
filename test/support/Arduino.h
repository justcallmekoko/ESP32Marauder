#pragma once

#include <stdint.h>

constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int INPUT = 0;
constexpr int INPUT_PULLUP = 1;
constexpr int INPUT_PULLDOWN = 2;

namespace arduino_test {
inline int read_value = LOW;
inline uint32_t current_millis = 0;
inline int pin_mode_pin = -1;
inline int pin_mode_value = -1;
}

inline void pinMode(int pin, int mode) {
  arduino_test::pin_mode_pin = pin;
  arduino_test::pin_mode_value = mode;
}
inline int digitalRead(int) { return arduino_test::read_value; }
inline uint32_t millis() { return arduino_test::current_millis; }

namespace arduino_test {
inline void reset() {
  read_value = LOW;
  current_millis = 0;
  pin_mode_pin = -1;
  pin_mode_value = -1;
}
inline void setDigitalRead(int value) { read_value = value; }
inline void setMillis(uint32_t value) { current_millis = value; }
inline int lastPinModePin() { return pin_mode_pin; }
inline int lastPinModeValue() { return pin_mode_value; }
}
