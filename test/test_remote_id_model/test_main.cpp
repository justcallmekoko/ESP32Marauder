#include <unity.h>

#include <cmath>
#include <cstring>

#include "RemoteIdModel.h"

void setUp() {}
void tearDown() {}

void test_layout_profiles() {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RemoteIdLayout::SplitListGrid),
                          static_cast<uint8_t>(remoteIdLayoutForDisplay(320, 240)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RemoteIdLayout::SplitListGrid),
                          static_cast<uint8_t>(remoteIdLayoutForDisplay(240, 320)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RemoteIdLayout::Compact),
                          static_cast<uint8_t>(remoteIdLayoutForDisplay(240, 135)));
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(RemoteIdLayout::ListOnly),
                          static_cast<uint8_t>(remoteIdLayoutForDisplay(240, 240)));
}

void test_store_deduplicates_and_merges_transports() {
  RemoteIdRecord records[2];
  RemoteIdStore store(records, 2);
  const uint8_t mac[6] = {1, 2, 3, 4, 5, 6};
  RemoteIdRecord& first = store.observe(mac, RemoteIdTransport::WifiBeacon, -70, 100);
  std::strncpy(first.uasId, "TEST-123", sizeof(first.uasId) - 1);
  first.hasUasId = true;
  RemoteIdRecord& second = store.observe(mac, RemoteIdTransport::BleLegacy, -55, 200);

  TEST_ASSERT_EQUAL_UINT32(1, store.size());
  TEST_ASSERT_EQUAL_UINT32(2, second.packetCount);
  TEST_ASSERT_EQUAL_INT8(-55, second.rssi);
  TEST_ASSERT_EQUAL_UINT8(5, second.transportMask);
  TEST_ASSERT_EQUAL_PTR(&second, store.findByUasId("TEST-123"));
  TEST_ASSERT_EQUAL_PTR(&second, store.findByMac(mac));
}

void test_store_evicts_oldest_record() {
  RemoteIdRecord records[2];
  RemoteIdStore store(records, 2);
  const uint8_t one[6] = {1, 0, 0, 0, 0, 0};
  const uint8_t two[6] = {2, 0, 0, 0, 0, 0};
  const uint8_t three[6] = {3, 0, 0, 0, 0, 0};
  store.observe(one, RemoteIdTransport::WifiBeacon, -60, 100);
  store.observe(two, RemoteIdTransport::WifiBeacon, -60, 200);
  store.observe(three, RemoteIdTransport::WifiBeacon, -60, 300);

  TEST_ASSERT_EQUAL_UINT32(2, store.size());
  TEST_ASSERT_EQUAL_UINT8(3, store.at(0)->mac[0]);
  TEST_ASSERT_EQUAL_UINT8(2, store.at(1)->mac[0]);
}

void test_projection_is_true_north_and_east_right() {
  const int32_t originLat = 400000000;
  const int32_t originLon = -740000000;
  RemoteIdGridPoint north = remoteIdProjectToGrid(
      originLat + 10000, originLon, originLat, originLon, 200, 100, 200.0f);
  RemoteIdGridPoint east = remoteIdProjectToGrid(
      originLat, originLon + 10000, originLat, originLon, 200, 100, 200.0f);

  TEST_ASSERT_TRUE(north.y < 50);
  TEST_ASSERT_INT_WITHIN(1, 100, north.x);
  TEST_ASSERT_TRUE(east.x > 100);
  TEST_ASSERT_INT_WITHIN(1, 50, east.y);
}

void test_projection_clamps_off_grid_points_to_edge() {
  RemoteIdGridPoint point = remoteIdProjectToGrid(
      410000000, -740000000, 400000000, -740000000, 200, 100, 50.0f, 8);
  TEST_ASSERT_FALSE(point.visible);
  TEST_ASSERT_TRUE(point.offGrid);
  TEST_ASSERT_EQUAL_INT16(8, point.y);
  TEST_ASSERT_TRUE(point.x >= 8 && point.x < 192);
}

void test_scale_includes_aircraft_and_operator() {
  RemoteIdRecord records[1];
  records[0].hasLocation = true;
  records[0].latitudeE7 = 400010000;
  records[0].longitudeE7 = -740000000;
  records[0].hasOperatorLocation = true;
  records[0].operatorLatitudeE7 = 400000000;
  records[0].operatorLongitudeE7 = -739980000;
  const float radius = remoteIdGridScaleMeters(records, 1, 400000000, -740000000);
  TEST_ASSERT_TRUE(radius > 180.0f);
}

void test_distance_and_bearing() {
  const int32_t lat = 400000000;
  const int32_t lon = -740000000;
  const float distance = remoteIdDistanceMeters(lat, lon, lat + 10000, lon);
  const float bearing = remoteIdBearingDegrees(lat, lon, lat + 10000, lon);
  TEST_ASSERT_FLOAT_WITHIN(2.0f, 111.2f, distance);
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, bearing);
}

void test_distance_formatter_uses_kilometers() {
  char text[16] = {};
  remoteIdFormatDistanceKm(1234.0f, text, sizeof(text));
  TEST_ASSERT_EQUAL_STRING("1.23km", text);
  remoteIdFormatDistanceKm(42.0f, text, sizeof(text));
  TEST_ASSERT_EQUAL_STRING("0.04km", text);
}

void test_grid_radius_formatter_switches_to_kilometers() {
  char text[16] = {};
  remoteIdFormatGridRadius(999.4f, text, sizeof(text));
  TEST_ASSERT_EQUAL_STRING("999m", text);
  remoteIdFormatGridRadius(1000.0f, text, sizeof(text));
  TEST_ASSERT_EQUAL_STRING("1.0km", text);
  remoteIdFormatGridRadius(2450.0f, text, sizeof(text));
  TEST_ASSERT_EQUAL_STRING("2.5km", text);
}

void test_stale_detection_handles_millis_wrap() {
  TEST_ASSERT_FALSE(remoteIdIsStale(500, 0xFFFFFF00U, 1000));
  TEST_ASSERT_TRUE(remoteIdIsStale(1000, 0xFFFFFF00U, 1000));
}

void test_store_prunes_expired_records_and_preserves_target() {
  RemoteIdRecord records[3];
  RemoteIdStore store(records, 3);
  const uint8_t expired[6] = {1, 0, 0, 0, 0, 0};
  const uint8_t target[6] = {2, 0, 0, 0, 0, 0};
  const uint8_t active[6] = {3, 0, 0, 0, 0, 0};
  store.observe(expired, RemoteIdTransport::WifiBeacon, -80, 100);
  RemoteIdRecord& selected =
      store.observe(target, RemoteIdTransport::WifiBeacon, -70, 100);
  std::strncpy(selected.uasId, "TARGET", sizeof(selected.uasId) - 1);
  selected.hasUasId = true;
  store.observe(active, RemoteIdTransport::BleLegacy, -60, 950);

  TEST_ASSERT_EQUAL_UINT32(1, store.pruneStale(1100, 500, "TARGET"));
  TEST_ASSERT_EQUAL_UINT32(2, store.size());
  TEST_ASSERT_NOT_NULL(store.findByUasId("TARGET"));
  TEST_ASSERT_NOT_NULL(store.findByMac(active));
  TEST_ASSERT_NULL(store.findByMac(expired));
}

void test_lifecycle_marks_lost_and_reacquired() {
  RemoteIdRecord records[1];
  RemoteIdStore store(records, 1);
  const uint8_t mac[6] = {1, 2, 3, 4, 5, 6};
  store.observe(mac, RemoteIdTransport::WifiBeacon, -70, 100);
  TEST_ASSERT_EQUAL_UINT32(1, store.updateLifecycle(1100, 1000));
  TEST_ASSERT_TRUE(store.at(0)->isLost);
  TEST_ASSERT_EQUAL_UINT16(1, store.at(0)->lostCount);
  TEST_ASSERT_EQUAL_UINT32(1100, store.at(0)->lastLostMs);
  store.observe(mac, RemoteIdTransport::BleLegacy, -60, 1200);
  TEST_ASSERT_FALSE(store.at(0)->isLost);
  TEST_ASSERT_EQUAL_UINT16(1, store.at(0)->reacquiredCount);
  TEST_ASSERT_EQUAL_UINT32(1200, store.at(0)->lastReacquiredMs);
}

void test_store_tracks_packet_rate_and_can_erase_duplicate_mac() {
  RemoteIdRecord records[2];
  RemoteIdStore store(records, 2);
  const uint8_t first[6] = {1, 0, 0, 0, 0, 0};
  const uint8_t second[6] = {2, 0, 0, 0, 0, 0};
  store.observe(first, RemoteIdTransport::WifiBeacon, -70, 100);
  store.observe(first, RemoteIdTransport::WifiBeacon, -70, 600);
  RemoteIdRecord& rated = store.observe(first, RemoteIdTransport::WifiBeacon, -70, 1100);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, rated.packetRateHz);
  RemoteIdRecord& kept = store.observe(second, RemoteIdTransport::BleLegacy, -60, 1200);
  TEST_ASSERT_TRUE(store.eraseByMac(first, &kept));
  TEST_ASSERT_EQUAL_UINT32(1, store.size());
  TEST_ASSERT_EQUAL_PTR(&records[0], store.findByMac(second));
}

void test_coordinate_validation_rejects_unavailable_and_out_of_range() {
  TEST_ASSERT_TRUE(remoteIdCoordinatesValid(407123456, -740123456));
  TEST_ASSERT_FALSE(remoteIdCoordinatesValid(0, 0));
  TEST_ASSERT_FALSE(remoteIdCoordinatesValid(900000001, 0));
  TEST_ASSERT_FALSE(remoteIdCoordinatesValid(0, 1800000001));
}

void test_basic_id_selection_prefers_stable_device_serial() {
  TEST_ASSERT_TRUE(remoteIdShouldReplaceBasicId(0, false, 2));
  TEST_ASSERT_TRUE(remoteIdShouldReplaceBasicId(2, true, 1));
  TEST_ASSERT_FALSE(remoteIdShouldReplaceBasicId(1, true, 2));
  TEST_ASSERT_TRUE(remoteIdShouldReplaceBasicId(1, true, 1));
}

void test_transport_formatter_handles_single_and_combined_methods() {
  char output[16];
  remoteIdFormatTransports(static_cast<uint8_t>(RemoteIdTransport::BleLegacy),
                           output, sizeof(output));
  TEST_ASSERT_EQUAL_STRING("B4", output);
  remoteIdFormatTransports(
      static_cast<uint8_t>(RemoteIdTransport::WifiBeacon) |
          static_cast<uint8_t>(RemoteIdTransport::BleLegacy),
      output, sizeof(output));
  TEST_ASSERT_EQUAL_STRING("WB+B4", output);
  remoteIdFormatTransports(0, output, sizeof(output));
  TEST_ASSERT_EQUAL_STRING("--", output);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_layout_profiles);
  RUN_TEST(test_store_deduplicates_and_merges_transports);
  RUN_TEST(test_store_evicts_oldest_record);
  RUN_TEST(test_projection_is_true_north_and_east_right);
  RUN_TEST(test_projection_clamps_off_grid_points_to_edge);
  RUN_TEST(test_scale_includes_aircraft_and_operator);
  RUN_TEST(test_distance_and_bearing);
  RUN_TEST(test_distance_formatter_uses_kilometers);
  RUN_TEST(test_grid_radius_formatter_switches_to_kilometers);
  RUN_TEST(test_stale_detection_handles_millis_wrap);
  RUN_TEST(test_store_prunes_expired_records_and_preserves_target);
  RUN_TEST(test_lifecycle_marks_lost_and_reacquired);
  RUN_TEST(test_store_tracks_packet_rate_and_can_erase_duplicate_mac);
  RUN_TEST(test_coordinate_validation_rejects_unavailable_and_out_of_range);
  RUN_TEST(test_basic_id_selection_prefers_stable_device_serial);
  RUN_TEST(test_transport_formatter_handles_single_and_combined_methods);
  return UNITY_END();
}
