#include "FoxHuntTarget.h"

namespace marauder {

bool foxHuntMacMatches(const uint8_t* target, const uint8_t* observed) {
  if ((target == nullptr) || (observed == nullptr))
    return false;

  for (uint8_t i = 0; i < kFoxHuntMacSize; i++) {
    if (target[i] != observed[i])
      return false;
  }
  return true;
}

bool foxHuntShouldUpdateChannel(bool bluetooth, uint8_t channel) {
  return !bluetooth && channel > 0;
}

bool foxHuntTargetIsStale(uint32_t now, uint32_t last_seen, uint32_t timeout) {
  return static_cast<uint32_t>(now - last_seen) > timeout;
}

uint8_t foxHuntNextChannel(uint8_t current, uint8_t maximum) {
  if (maximum == 0)
    return 0;
  if ((current == 0) || (current >= maximum))
    return 1;
  return current + 1;
}

}  // namespace marauder
