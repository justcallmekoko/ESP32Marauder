#pragma once

#include <stdint.h>

class MenuInputRepeat {
 public:
  static constexpr uint32_t kHoldDelayMs = 1000;
  static constexpr uint32_t kRepeatIntervalMs = 120;

  bool update(bool pressed, uint32_t now) {
    if (!pressed) {
      active_ = false;
      return false;
    }
    if (!active_) {
      active_ = true;
      pressed_at_ = now;
      repeated_at_ = now;
      return true;
    }
    if (static_cast<uint32_t>(now - pressed_at_) < kHoldDelayMs ||
        static_cast<uint32_t>(now - repeated_at_) < kRepeatIntervalMs) {
      return false;
    }
    repeated_at_ = now;
    return true;
  }

  void reset() { active_ = false; }

 private:
  bool active_ = false;
  uint32_t pressed_at_ = 0;
  uint32_t repeated_at_ = 0;
};
