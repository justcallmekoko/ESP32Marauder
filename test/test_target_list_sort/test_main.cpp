#include <unity.h>

#include <string.h>
#include <vector>

#include "TargetListSort.h"

static TargetListItem item(size_t index, int rssi, uint8_t channel, uint32_t seen, const char* name) {
  TargetListItem value = {index, (int16_t)rssi, channel, seen, {}};
  strncpy(value.name, name, sizeof(value.name) - 1);
  return value;
}

void test_signal_sort_is_strongest_first_and_stable() {
  std::vector<TargetListItem> values = {item(0, -70, 6, 1, "Zulu"), item(1, -40, 1, 1, "Beta"), item(2, -40, 11, 1, "Alpha")};
  sortTargetList(values, TargetSortMode::SIGNAL_DESC);
  TEST_ASSERT_EQUAL_UINT16(2, values[0].source_index);
  TEST_ASSERT_EQUAL_UINT16(1, values[1].source_index);
  TEST_ASSERT_EQUAL_UINT16(0, values[2].source_index);
}

void test_name_sort_is_case_insensitive() {
  std::vector<TargetListItem> values = {item(0, -1, 1, 1, "zulu"), item(1, -1, 1, 1, "Alpha")};
  sortTargetList(values, TargetSortMode::NAME_ASC);
  TEST_ASSERT_EQUAL_UINT16(1, values[0].source_index);
}

void test_channel_sort_groups_low_to_high() {
  std::vector<TargetListItem> values = {item(0, -1, 149, 1, "A"), item(1, -1, 6, 1, "B")};
  sortTargetList(values, TargetSortMode::CHANNEL_ASC);
  TEST_ASSERT_EQUAL_UINT16(1, values[0].source_index);
}

void test_filters_handle_recency_rollover_and_bands() {
  TargetListItem recent = item(0, -1, 6, 0xfffffff0u, "A");
  TEST_ASSERT_TRUE(targetListItemMatchesFilter(recent, TargetFilterMode::RECENT_30S, 0x20u));
  TEST_ASSERT_TRUE(targetListItemMatchesFilter(recent, TargetFilterMode::BAND_24_GHZ, 0));
  TEST_ASSERT_FALSE(targetListItemMatchesFilter(recent, TargetFilterMode::BAND_5_GHZ, 0));
  TargetListItem five = item(1, -1, 149, 1, "B");
  TEST_ASSERT_TRUE(targetListItemMatchesFilter(five, TargetFilterMode::BAND_5_GHZ, 0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_signal_sort_is_strongest_first_and_stable);
  RUN_TEST(test_name_sort_is_case_insensitive);
  RUN_TEST(test_channel_sort_groups_low_to_high);
  RUN_TEST(test_filters_handle_recency_rollover_and_bands);
  return UNITY_END();
}

