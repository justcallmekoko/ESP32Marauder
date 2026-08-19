#pragma once

#include <stddef.h>
#include <stdint.h>

enum class ReconSource : uint8_t { AP_LIST = 0, STATION_LIST = 1, BLE_LIST = 2 };

struct ReconRange {
  size_t begin;
  size_t end;
};

inline uint8_t reconRssiLevel(int8_t rssi) {
  if (rssi <= -127) return 0;
  if (rssi <= -100) return 1;
  if (rssi >= -30) return 8;
  return static_cast<uint8_t>(1 + ((rssi + 100) * 7) / 70);
}

class ReconMissionState {
 public:
  void reset() {
    for (size_t& cursor : cursors) cursor = 0;
  }

  ReconRange consume(ReconSource source, size_t size) {
    size_t& cursor = cursors[static_cast<uint8_t>(source)];
    if (size < cursor) cursor = 0;
    const ReconRange range = {cursor, size};
    cursor = size;
    return range;
  }

 private:
  size_t cursors[3] = {0, 0, 0};
};
