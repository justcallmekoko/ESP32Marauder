#pragma once

#include <stddef.h>
#include <stdint.h>

namespace marauder {

struct IBeaconPayload {
  uint8_t uuid[16] = {};
  uint16_t major = 0;
  uint16_t minor = 0;
  int8_t measured_power = 0;
};

inline bool sameIBeacon(const IBeaconPayload& lhs, const IBeaconPayload& rhs) {
  if (lhs.major != rhs.major || lhs.minor != rhs.minor) return false;
  for (size_t index = 0; index < sizeof(lhs.uuid); index++) {
    if (lhs.uuid[index] != rhs.uuid[index]) return false;
  }
  return true;
}

inline bool parseIBeacon(const uint8_t* payload, size_t length, IBeaconPayload& beacon) {
  if (!payload || length < 25) return false;
  for (size_t offset = 0; offset + 25 <= length; offset++) {
    if (payload[offset] != 0x4c || payload[offset + 1] != 0x00 ||
        payload[offset + 2] != 0x02 || payload[offset + 3] != 0x15)
      continue;
    for (size_t index = 0; index < sizeof(beacon.uuid); index++)
      beacon.uuid[index] = payload[offset + 4 + index];
    beacon.major = (static_cast<uint16_t>(payload[offset + 20]) << 8) |
                   payload[offset + 21];
    beacon.minor = (static_cast<uint16_t>(payload[offset + 22]) << 8) |
                   payload[offset + 23];
    beacon.measured_power = static_cast<int8_t>(payload[offset + 24]);
    return true;
  }
  return false;
}

}  // namespace marauder
