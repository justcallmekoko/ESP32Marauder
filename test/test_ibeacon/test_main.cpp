#include <unity.h>

#include "IBeacon.h"

namespace {

void test_parses_ibeacon_manufacturer_payload() {
  const uint8_t payload[] = {
    0x02, 0x01, 0x06, 0x1a, 0xff, 0x4c, 0x00, 0x02, 0x15,
    0xe2, 0xc5, 0x6d, 0xb5, 0xdf, 0xfb, 0x48, 0xd2,
    0xb0, 0x60, 0xd0, 0xf5, 0xa7, 0x10, 0x96, 0xe0,
    0x12, 0x34, 0xab, 0xcd, 0xc5
  };
  marauder::IBeaconPayload beacon;
  TEST_ASSERT_TRUE(marauder::parseIBeacon(payload, sizeof(payload), beacon));
  TEST_ASSERT_EQUAL_HEX8(0xe2, beacon.uuid[0]);
  TEST_ASSERT_EQUAL_HEX8(0xe0, beacon.uuid[15]);
  TEST_ASSERT_EQUAL_HEX16(0x1234, beacon.major);
  TEST_ASSERT_EQUAL_HEX16(0xabcd, beacon.minor);
  TEST_ASSERT_EQUAL_INT8(-59, beacon.measured_power);
}

void test_rejects_truncated_or_wrong_company_payload() {
  const uint8_t truncated[] = {0x4c, 0x00, 0x02, 0x15, 0x00};
  uint8_t wrong[25] = {0x4d, 0x00, 0x02, 0x15};
  marauder::IBeaconPayload beacon;
  TEST_ASSERT_FALSE(marauder::parseIBeacon(truncated, sizeof(truncated), beacon));
  TEST_ASSERT_FALSE(marauder::parseIBeacon(wrong, sizeof(wrong), beacon));
}

void test_finds_signature_after_advertising_headers() {
  uint8_t payload[31] = {};
  payload[6] = 0x4c;
  payload[7] = 0x00;
  payload[8] = 0x02;
  payload[9] = 0x15;
  payload[29] = 0x00;
  payload[30] = 0xc0;
  marauder::IBeaconPayload beacon;
  TEST_ASSERT_TRUE(marauder::parseIBeacon(payload, sizeof(payload), beacon));
  TEST_ASSERT_EQUAL_INT8(-64, beacon.measured_power);
}

}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parses_ibeacon_manufacturer_payload);
  RUN_TEST(test_rejects_truncated_or_wrong_company_payload);
  RUN_TEST(test_finds_signature_after_advertising_headers);
  return UNITY_END();
}
