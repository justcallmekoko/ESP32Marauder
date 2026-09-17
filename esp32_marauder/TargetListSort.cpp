#include "TargetListSort.h"

#include <ctype.h>
#include <string.h>

namespace {
int caseInsensitiveCompare(const char* left, const char* right) {
  while (*left && *right) {
    int a = tolower((unsigned char)*left++);
    int b = tolower((unsigned char)*right++);
    if (a != b)
      return a - b;
  }
  return (unsigned char)*left - (unsigned char)*right;
}

bool targetComesBefore(const TargetListItem& a, const TargetListItem& b,
                       TargetSortMode mode) {
  switch (mode) {
    case TargetSortMode::SIGNAL_DESC:
      return a.rssi != b.rssi ? a.rssi > b.rssi
                              : caseInsensitiveCompare(a.name, b.name) < 0;
    case TargetSortMode::NAME_ASC:
      return caseInsensitiveCompare(a.name, b.name) < 0;
    case TargetSortMode::CHANNEL_ASC:
      return a.channel != b.channel ? a.channel < b.channel
                                    : caseInsensitiveCompare(a.name, b.name) < 0;
  }
  return false;
}
}  // namespace

bool targetListItemMatchesFilter(const TargetListItem& item, TargetFilterMode filter,
                                 uint32_t now_ms) {
  switch (filter) {
    case TargetFilterMode::ALL:
      return true;
    case TargetFilterMode::RECENT_30S:
      return item.last_seen_ms != 0 && (uint32_t)(now_ms - item.last_seen_ms) <= 30000;
    case TargetFilterMode::BAND_24_GHZ:
      return item.channel >= 1 && item.channel <= 14;
    case TargetFilterMode::BAND_5_GHZ:
      return item.channel > 14;
  }
  return true;
}

void sortTargetList(std::vector<TargetListItem>& items, TargetSortMode mode) {
  // Target lists are screen-sized. Stable insertion sort avoids pulling the
  // large libstdc++ stable_sort implementation into constrained firmware.
  for (size_t i = 1; i < items.size(); ++i) {
    TargetListItem item = items[i];
    size_t position = i;
    while (position > 0 && targetComesBefore(item, items[position - 1], mode)) {
      items[position] = items[position - 1];
      --position;
    }
    items[position] = item;
  }
}
