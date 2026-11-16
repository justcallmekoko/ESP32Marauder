#include <unity.h>

#include <cstring>
#include <limits>

#include "BeaconFrame.h"

void setUp() {}
void tearDown() {}

void test_channel_is_written_after_ssid() {
  uint8_t frame[64] = {};
  TEST_ASSERT_TRUE(setBeaconFrameChannel(frame, sizeof(frame), 4, 11));
  TEST_ASSERT_EQUAL_UINT8(11, frame[54]);
}

void test_adjacent_frame_bytes_are_preserved() {
  uint8_t frame[64] = {};
  frame[53] = 0xAA;
  frame[55] = 0xBB;
  TEST_ASSERT_TRUE(setBeaconFrameChannel(frame, sizeof(frame), 4, 6));
  TEST_ASSERT_EQUAL_HEX8(0xAA, frame[53]);
  TEST_ASSERT_EQUAL_UINT8(6, frame[54]);
  TEST_ASSERT_EQUAL_HEX8(0xBB, frame[55]);
}

void test_short_frame_is_rejected() {
  uint8_t frame[54] = {};
  TEST_ASSERT_FALSE(setBeaconFrameChannel(frame, sizeof(frame), 4, 1));
}

void test_null_frame_is_rejected() {
  TEST_ASSERT_FALSE(setBeaconFrameChannel(nullptr, 64, 4, 1));
}

namespace {

marauder::BeaconIdentity makeIdentity(uint8_t channel) {
  marauder::BeaconIdentity identity;
  const char* ssid = "Visible Test AP";
  identity.ssid_length = strlen(ssid);
  memcpy(identity.ssid, ssid, identity.ssid_length + 1);
  const uint8_t bssid[] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55};
  memcpy(identity.bssid, bssid, sizeof(bssid));
  identity.channel = channel;
  identity.sequence = 0x123;
  return identity;
}

const uint8_t* findInformationElement(
  const uint8_t* frame,
  size_t frame_length,
  uint8_t tag
) {
  size_t offset = 36;
  while (offset + 2 <= frame_length) {
    const uint8_t element_tag = frame[offset];
    const uint8_t element_length = frame[offset + 1];
    if (offset + 2 + element_length > frame_length)
      return nullptr;
    if (element_tag == tag)
      return frame + offset;
    offset += 2 + element_length;
  }
  return nullptr;
}

}  // namespace

void test_24ghz_open_beacon_has_exact_valid_structure() {
  uint8_t frame[marauder::kMaxOpenBeaconFrameLength];
  const marauder::BeaconIdentity identity = makeIdentity(6);
  const size_t frame_length = marauder::buildOpenBeaconFrame(
    frame,
    sizeof(frame),
    identity,
    0x0102030405060708ULL
  );

  TEST_ASSERT_EQUAL_UINT32(57 + identity.ssid_length, frame_length);
  TEST_ASSERT_EQUAL_HEX8(0x80, frame[0]);
  TEST_ASSERT_EQUAL_HEX8(0x01, frame[34]);
  TEST_ASSERT_EQUAL_HEX8(0x00, frame[35]);
  TEST_ASSERT_EQUAL_UINT8(identity.ssid_length, frame[37]);
  TEST_ASSERT_EQUAL_MEMORY(identity.ssid, frame + 38, identity.ssid_length);
  TEST_ASSERT_EQUAL_MEMORY(identity.bssid, frame + 10, sizeof(identity.bssid));
  TEST_ASSERT_EQUAL_MEMORY(identity.bssid, frame + 16, sizeof(identity.bssid));
  TEST_ASSERT_EQUAL_HEX8(0x30, frame[22]);
  TEST_ASSERT_EQUAL_HEX8(0x12, frame[23]);

  const uint8_t* ds = findInformationElement(frame, frame_length, 0x03);
  TEST_ASSERT_NOT_NULL(ds);
  TEST_ASSERT_EQUAL_UINT8(1, ds[1]);
  TEST_ASSERT_EQUAL_UINT8(6, ds[2]);
  TEST_ASSERT_NOT_NULL(findInformationElement(frame, frame_length, 0x32));
  TEST_ASSERT_NULL(findInformationElement(frame, frame_length, 0x30));
}

void test_5ghz_open_beacon_uses_ofdm_rates_without_ds_parameter() {
  uint8_t frame[marauder::kMaxOpenBeaconFrameLength];
  const marauder::BeaconIdentity identity = makeIdentity(149);
  const size_t frame_length = marauder::buildOpenBeaconFrame(
    frame,
    sizeof(frame),
    identity,
    123456
  );

  TEST_ASSERT_EQUAL_UINT32(48 + identity.ssid_length, frame_length);
  const uint8_t* rates = findInformationElement(frame, frame_length, 0x01);
  TEST_ASSERT_NOT_NULL(rates);
  TEST_ASSERT_EQUAL_UINT8(8, rates[1]);
  TEST_ASSERT_EQUAL_HEX8(0x8c, rates[2]);
  TEST_ASSERT_NULL(findInformationElement(frame, frame_length, 0x03));
  TEST_ASSERT_NULL(findInformationElement(frame, frame_length, 0x32));
  TEST_ASSERT_NULL(findInformationElement(frame, frame_length, 0x30));
}

void test_beacon_builder_rejects_short_output_buffer() {
  uint8_t frame[48];
  TEST_ASSERT_EQUAL_UINT32(
    0,
    marauder::buildOpenBeaconFrame(frame, sizeof(frame), makeIdentity(1), 0)
  );
}

void test_bssid_is_normalized_to_local_unicast() {
  uint8_t bssid[] = {0xff, 0xaa, 0xbb, 0xcc, 0xdd, 0xee};
  marauder::makeLocalUnicastBssid(bssid);
  TEST_ASSERT_EQUAL_HEX8(0xfe, bssid[0]);
  TEST_ASSERT_EQUAL_UINT8(0, bssid[0] & 0x01);
  TEST_ASSERT_EQUAL_UINT8(0x02, bssid[0] & 0x02);
}

void test_beacon_cycle_expiry_is_millis_wrap_safe() {
  TEST_ASSERT_EQUAL_UINT32(30, marauder::kRandomBeaconIdentityCount);
  TEST_ASSERT_EQUAL_UINT32(10000, marauder::kRandomBeaconCycleMs);
  TEST_ASSERT_FALSE(marauder::beaconCycleExpired(10999, 1000, 10000));
  TEST_ASSERT_TRUE(marauder::beaconCycleExpired(11000, 1000, 10000));
  TEST_ASSERT_TRUE(marauder::beaconCycleExpired(
    600,
    std::numeric_limits<uint32_t>::max() - 500,
    1000
  ));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_channel_is_written_after_ssid);
  RUN_TEST(test_adjacent_frame_bytes_are_preserved);
  RUN_TEST(test_short_frame_is_rejected);
  RUN_TEST(test_null_frame_is_rejected);
  RUN_TEST(test_24ghz_open_beacon_has_exact_valid_structure);
  RUN_TEST(test_5ghz_open_beacon_uses_ofdm_rates_without_ds_parameter);
  RUN_TEST(test_beacon_builder_rejects_short_output_buffer);
  RUN_TEST(test_bssid_is_normalized_to_local_unicast);
  RUN_TEST(test_beacon_cycle_expiry_is_millis_wrap_safe);
  return UNITY_END();
}
