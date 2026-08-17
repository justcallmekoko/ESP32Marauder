#include "TargetListSort.h"

#include <algorithm>
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
  std::stable_sort(items.begin(), items.end(), [mode](const TargetListItem& a, const TargetListItem& b) {
    switch (mode) {
      case TargetSortMode::SIGNAL_DESC:
        return a.rssi != b.rssi ? a.rssi > b.rssi : caseInsensitiveCompare(a.name, b.name) < 0;
      case TargetSortMode::NAME_ASC:
        return caseInsensitiveCompare(a.name, b.name) < 0;
      case TargetSortMode::CHANNEL_ASC:
        return a.channel != b.channel ? a.channel < b.channel : caseInsensitiveCompare(a.name, b.name) < 0;
    }
    return false;
  });
}

