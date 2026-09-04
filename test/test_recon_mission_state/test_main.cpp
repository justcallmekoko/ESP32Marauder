#include <unity.h>

#include "ReconMissionState.h"

void test_consumes_only_new_entries() {
  ReconMissionState state;
  ReconRange first = state.consume(ReconSource::AP_LIST, 4);
  ReconRange second = state.consume(ReconSource::AP_LIST, 6);
  TEST_ASSERT_EQUAL_UINT32(0, first.begin);
  TEST_ASSERT_EQUAL_UINT32(4, first.end);
  TEST_ASSERT_EQUAL_UINT32(4, second.begin);
  TEST_ASSERT_EQUAL_UINT32(6, second.end);
}

void test_handles_source_lists_being_cleared() {
  ReconMissionState state;
  state.consume(ReconSource::BLE_LIST, 5);
  ReconRange after_clear = state.consume(ReconSource::BLE_LIST, 2);
  TEST_ASSERT_EQUAL_UINT32(0, after_clear.begin);
  TEST_ASSERT_EQUAL_UINT32(2, after_clear.end);
}

void test_tracks_sources_independently() {
  ReconMissionState state;
  state.consume(ReconSource::AP_LIST, 3);
  ReconRange stations = state.consume(ReconSource::STATION_LIST, 2);
  TEST_ASSERT_EQUAL_UINT32(0, stations.begin);
  TEST_ASSERT_EQUAL_UINT32(2, stations.end);
}

void test_maps_rssi_to_eight_non_directional_levels() {
  TEST_ASSERT_EQUAL_UINT8(0, reconRssiLevel(-128));
  TEST_ASSERT_EQUAL_UINT8(1, reconRssiLevel(-100));
  TEST_ASSERT_EQUAL_UINT8(4, reconRssiLevel(-65));
  TEST_ASSERT_EQUAL_UINT8(8, reconRssiLevel(-30));
  TEST_ASSERT_EQUAL_UINT8(8, reconRssiLevel(-10));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_consumes_only_new_entries);
  RUN_TEST(test_handles_source_lists_being_cleared);
  RUN_TEST(test_tracks_sources_independently);
  RUN_TEST(test_maps_rssi_to_eight_non_directional_levels);
  return UNITY_END();
}
