#pragma once

#include <stddef.h>
#include <stdint.h>

namespace marauder {

constexpr size_t kMaxBeaconSsidLength = 32;
constexpr size_t kMaxOpenBeaconFrameLength = 96;
constexpr size_t kRandomBeaconIdentityCount = 30;
constexpr uint32_t kRandomBeaconCycleMs = 10000;

struct BeaconIdentity {
  char ssid[kMaxBeaconSsidLength + 1] = {};
  uint8_t ssid_length = 0;
  uint8_t bssid[6] = {};
  uint8_t channel = 1;
  uint16_t sequence = 0;
};

bool isFiveGhzBeaconChannel(uint8_t channel);
void makeLocalUnicastBssid(uint8_t bssid[6]);

bool beaconCycleExpired(
  uint32_t current_time,
  uint32_t cycle_started,
  uint32_t cycle_duration
);

size_t buildOpenBeaconFrame(
  uint8_t* frame,
  size_t frame_capacity,
  const BeaconIdentity& identity,
  uint64_t timestamp_us
);

}  // namespace marauder

bool setBeaconFrameChannel(
  uint8_t* frame,
  size_t frame_size,
  size_t ssid_length,
  uint8_t channel
);
