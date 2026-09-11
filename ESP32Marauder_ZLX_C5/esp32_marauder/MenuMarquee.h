#pragma once

#include <stdint.h>

namespace MenuMarquee {

constexpr uint32_t START_DELAY_MS = 1000;
constexpr uint32_t STEP_INTERVAL_MS = 150;
constexpr uint16_t END_PAUSE_STEPS = 5;

inline uint16_t offsetForElapsed(uint32_t elapsed_ms, uint16_t max_offset) {
  if (max_offset == 0 || elapsed_ms <= START_DELAY_MS)
    return 0;

  const uint32_t scroll_step = 1 + ((elapsed_ms - START_DELAY_MS - 1) / STEP_INTERVAL_MS);
  const uint32_t cycle_steps = static_cast<uint32_t>(max_offset) + 1 + END_PAUSE_STEPS;
  const uint32_t cycle_step = scroll_step % cycle_steps;

  if (cycle_step > max_offset)
    return max_offset;

  return static_cast<uint16_t>(cycle_step);
}

}  // namespace MenuMarquee
