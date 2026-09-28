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

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_layout_profiles);
  RUN_TEST(test_store_deduplicates_and_merges_transports);
  RUN_TEST(test_store_evicts_oldest_record);
  RUN_TEST(test_projection_is_true_north_and_east_right);
  RUN_TEST(test_scale_includes_aircraft_and_operator);
  RUN_TEST(test_distance_and_bearing);
  return UNITY_END();
}
