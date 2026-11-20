#pragma once

#include <stddef.h>
#include <stdint.h>
#include <vector>

enum class TargetSortMode : uint8_t {
  SIGNAL_DESC,
  NAME_ASC,
  CHANNEL_ASC,
};

enum class TargetFilterMode : uint8_t {
  ALL,
  RECENT_30S,
  BAND_24_GHZ,
  BAND_5_GHZ,
};

struct TargetListItem {
  size_t source_index;
  int16_t rssi;
  uint8_t channel;
  uint32_t last_seen_ms;
  char name[40];
};

bool targetListItemMatchesFilter(const TargetListItem& item, TargetFilterMode filter,
                                 uint32_t now_ms);
void sortTargetList(std::vector<TargetListItem>& items, TargetSortMode mode);

