#pragma once

#include <stddef.h>
#include <stdint.h>

enum class ReconSource : uint8_t { AP_LIST = 0, STATION_LIST = 1, BLE_LIST = 2 };

struct ReconRange {
  size_t begin;
  size_t end;
};

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
