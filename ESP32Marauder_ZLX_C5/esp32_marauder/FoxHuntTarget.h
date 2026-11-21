#pragma once

#include <stdint.h>

namespace marauder {

constexpr uint8_t kFoxHuntMacSize = 6;

bool foxHuntMacMatches(const uint8_t* target, const uint8_t* observed);
bool foxHuntShouldUpdateChannel(bool bluetooth, uint8_t channel);
bool foxHuntTargetIsStale(uint32_t now, uint32_t last_seen, uint32_t timeout);
uint8_t foxHuntNextChannel(uint8_t current, uint8_t maximum);

}  // namespace marauder
