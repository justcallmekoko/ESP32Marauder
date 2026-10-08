#include <unity.h>

#include <cstring>

#include "RemoteIdDecoder.h"

void setUp() {}
void tearDown() {}

static void writeI32Le(uint8_t* output, int32_t value) {
  const uint32_t raw = static_cast<uint32_t>(value);
  output[0] = raw & 0xFF;
  output[1] = (raw >> 8) & 0xFF;
  output[2] = (raw >> 16) & 0xFF;
  output[3] = (raw >> 24) & 0xFF;
}

static void writeU16Le(uint8_t* output, uint16_t value) {
  output[0] = value & 0xFF;
  output[1] = value >> 8;
}

static void writeU32Le(uint8_t* output, uint32_t value) {
  output[0] = value & 0xFF;
  output[1] = (value >> 8) & 0xFF;
  output[2] = (value >> 16) & 0xFF;
  output[3] = (value >> 24) & 0xFF;
}

static void makeBasicId(uint8_t* message) {
  std::memset(message, 0, RemoteIdDecoder::kMessageSize);
  message[0] = 0x02;
  message[1] = 0x14;
  std::memcpy(message + 2, "USS-Enterprise", 14);
}

static void makeLocation(uint8_t* message) {
  std::memset(message, 0, RemoteIdDecoder::kMessageSize);
  message[0] = 0x12;
  message[1] = 0x20;
  message[2] = 90;
  message[3] = 40;
  message[4] = static_cast<uint8_t>(-4);
  writeI32Le(message + 5, 407123456);
  writeI32Le(message + 9, -740123456);
  writeU16Le(message + 13, 2200);
  writeU16Le(message + 15, 2300);
  writeU16Le(message + 17, 2100);
  message[19] = 0xA7;
  message[20] = 0x04;
  writeU16Le(message + 21, 1234);
}

void test_decodes_message_pack_fields() {
  uint8_t payload[3 + 4 * RemoteIdDecoder::kMessageSize] = {};
  payload[0] = 0xF2;
  payload[1] = RemoteIdDecoder::kMessageSize;
  payload[2] = 4;
  makeBasicId(payload + 3);
  makeLocation(payload + 28);
  uint8_t* system = payload + 53;
  system[0] = 0x42;
  system[1] = 0x01;
  writeI32Le(system + 2, 407000000);
  writeI32Le(system + 6, -740000000);
  writeU16Le(system + 10, 3);
  system[12] = 12;
  writeU16Le(system + 13, 2400);
  writeU16Le(system + 15, 2200);
  system[17] = 0x21;
  writeU16Le(system + 18, 2300);
  writeU32Le(system + 20, 987654);
  uint8_t* operatorId = payload + 78;
  operatorId[0] = 0x52;
  operatorId[1] = 0;
  std::memcpy(operatorId + 2, "OPERATOR-42", 11);

  RemoteIdRecord record;
  RemoteIdDecoder decoder;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(RemoteIdDecodeResult::Decoded),
      static_cast<uint8_t>(decoder.decodePayload(payload, sizeof(payload), record)));
  TEST_ASSERT_EQUAL_STRING("USS-Enterprise", record.uasId);
  TEST_ASSERT_EQUAL_UINT8(1, record.idType);
  TEST_ASSERT_EQUAL_UINT8(4, record.uaType);
  TEST_ASSERT_EQUAL_INT32(407123456, record.latitudeE7);
  TEST_ASSERT_EQUAL_INT32(-740123456, record.longitudeE7);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, record.horizontalSpeedMps);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -2.0f, record.verticalSpeedMps);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 150.0f, record.altitudeGeoM);
  TEST_ASSERT_EQUAL_UINT8(7, record.horizontalAccuracy);
  TEST_ASSERT_EQUAL_UINT8(10, record.verticalAccuracy);
  TEST_ASSERT_EQUAL_UINT8(4, record.speedAccuracy);
  TEST_ASSERT_EQUAL_UINT16(1234, record.locationTimestampDeciseconds);
  TEST_ASSERT_EQUAL_INT32(407000000, record.operatorLatitudeE7);
  TEST_ASSERT_EQUAL_UINT16(3, record.areaCount);
  TEST_ASSERT_EQUAL_UINT16(120, record.areaRadiusM);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 200.0f, record.areaCeilingM);
  TEST_ASSERT_EQUAL_UINT8(2, record.categoryEu);
  TEST_ASSERT_EQUAL_UINT8(1, record.classEu);
  TEST_ASSERT_EQUAL_UINT32(987654, record.systemTimestamp);
  TEST_ASSERT_EQUAL_STRING("OPERATOR-42", record.operatorId);
}

void test_decodes_wifi_beacon_vendor_element() {
  uint8_t frame[36 + 2 + 5 + RemoteIdDecoder::kMessageSize] = {};
  frame[0] = 0x80;
  const size_t offset = 36;
  frame[offset] = 221;
  frame[offset + 1] = 5 + RemoteIdDecoder::kMessageSize;
  frame[offset + 2] = 0xFA;
  frame[offset + 3] = 0x0B;
  frame[offset + 4] = 0xBC;
  frame[offset + 5] = 0x0D;
  frame[offset + 6] = 7;
  makeBasicId(frame + offset + 7);

  RemoteIdRecord record;
  RemoteIdDecoder decoder;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(RemoteIdDecodeResult::Decoded),
      static_cast<uint8_t>(decoder.decodeWifiBeacon(frame, sizeof(frame), record)));
  TEST_ASSERT_EQUAL_STRING("USS-Enterprise", record.uasId);
}

void test_decodes_ble_service_data() {
  uint8_t data[4 + RemoteIdDecoder::kMessageSize] = {0xFA, 0xFF, 0x0D, 9};
  makeLocation(data + 4);
  RemoteIdRecord record;
  RemoteIdDecoder decoder;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(RemoteIdDecodeResult::Decoded),
      static_cast<uint8_t>(decoder.decodeBleServiceData(data, sizeof(data), record)));
  TEST_ASSERT_TRUE(record.hasLocation);
  TEST_ASSERT_EQUAL_INT32(407123456, record.latitudeE7);
  TEST_ASSERT_EQUAL_INT32(-740123456, record.longitudeE7);
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, record.horizontalSpeedMps);
}

void test_rejects_ble_service_data_without_open_drone_id_app_code() {
  uint8_t data[4 + RemoteIdDecoder::kMessageSize] = {0xFA, 0xFF, 0x01, 9};
  makeLocation(data + 4);
  RemoteIdRecord record;
  RemoteIdDecoder decoder;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(RemoteIdDecodeResult::Ignored),
      static_cast<uint8_t>(decoder.decodeBleServiceData(data, sizeof(data), record)));
}

void test_rejects_truncated_pack_and_ie() {
  RemoteIdRecord record;
  RemoteIdDecoder decoder;
  const uint8_t shortPack[] = {0xF2, 25, 2, 0x02};
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(RemoteIdDecodeResult::Malformed),
      static_cast<uint8_t>(decoder.decodePayload(shortPack, sizeof(shortPack), record)));

  uint8_t frame[38] = {};
  frame[0] = 0x80;
  frame[36] = 221;
  frame[37] = 20;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(RemoteIdDecodeResult::Malformed),
      static_cast<uint8_t>(decoder.decodeWifiBeacon(frame, sizeof(frame), record)));
}

void test_all_truncated_message_lengths_are_rejected() {
  uint8_t message[RemoteIdDecoder::kMessageSize] = {};
  makeBasicId(message);
  RemoteIdDecoder decoder;
  for (size_t length = 0; length < RemoteIdDecoder::kMessageSize; ++length) {
    RemoteIdRecord record;
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<uint8_t>(RemoteIdDecodeResult::Malformed),
        static_cast<uint8_t>(decoder.decodeMessage(message, length, record)));
  }
}

void test_rejects_out_of_range_location_for_plotting() {
  uint8_t message[RemoteIdDecoder::kMessageSize] = {};
  makeLocation(message);
  writeI32Le(message + 5, 900000001);
  RemoteIdRecord record;
  RemoteIdDecoder decoder;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(RemoteIdDecodeResult::Decoded),
      static_cast<uint8_t>(decoder.decodeMessage(message, sizeof(message), record)));
  TEST_ASSERT_FALSE(record.hasLocation);
}

void test_decodes_authentication_and_location_sentinels() {
  uint8_t auth[RemoteIdDecoder::kMessageSize] = {};
  auth[0] = 0x22;
  auth[1] = 0x30;
  auth[2] = 2;
  auth[3] = 40;
  writeU32Le(auth + 4, 123456);
  RemoteIdRecord record;
  RemoteIdDecoder decoder;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(RemoteIdDecodeResult::Decoded),
      static_cast<uint8_t>(decoder.decodeMessage(auth, sizeof(auth), record)));
  TEST_ASSERT_TRUE(record.hasAuthentication);
  TEST_ASSERT_EQUAL_UINT8(3, record.authenticationType);
  TEST_ASSERT_EQUAL_UINT8(2, record.authenticationLastPage);
  TEST_ASSERT_EQUAL_UINT32(123456, record.authenticationTimestamp);

  uint8_t location[RemoteIdDecoder::kMessageSize] = {};
  makeLocation(location);
  location[1] |= 0x03;
  location[2] = 181;
  location[3] = 255;
  location[4] = 126;
  writeU16Le(location + 15, 0);
  writeU16Le(location + 17, 0);
  writeU16Le(location + 21, 0xFFFF);
  record = RemoteIdRecord{};
  decoder.decodeMessage(location, sizeof(location), record);
  TEST_ASSERT_FALSE(record.directionValid);
  TEST_ASSERT_FALSE(record.horizontalSpeedValid);
  TEST_ASSERT_FALSE(record.verticalSpeedValid);
  TEST_ASSERT_FALSE(record.altitudeGeoValid);
  TEST_ASSERT_FALSE(record.heightValid);
  TEST_ASSERT_FALSE(record.locationTimestampValid);
}

void test_decodes_wifi_nan_action_frame() {
  uint8_t frame[44 + RemoteIdDecoder::kMessageSize] = {};
  frame[0] = 0xD0;
  const uint8_t destination[] = {0x51, 0x6F, 0x9A, 0x01, 0x00, 0x00};
  std::memcpy(frame + 4, destination, sizeof(destination));
  frame[24] = 0x04;
  frame[25] = 0x09;
  frame[26] = 0x50;
  frame[27] = 0x6F;
  frame[28] = 0x9A;
  frame[29] = 0x13;
  frame[30] = 0x03;
  frame[31] = 36;
  frame[33] = 0x88;
  frame[34] = 0x69;
  frame[35] = 0x19;
  frame[36] = 0x9D;
  frame[37] = 0x92;
  frame[38] = 0x09;
  frame[39] = 0x01;
  frame[41] = 0x10;
  frame[42] = 1 + RemoteIdDecoder::kMessageSize;
  frame[43] = 4;
  makeBasicId(frame + 44);
  RemoteIdRecord record;
  RemoteIdDecoder decoder;
  TEST_ASSERT_EQUAL_UINT8(
      static_cast<uint8_t>(RemoteIdDecodeResult::Decoded),
      static_cast<uint8_t>(decoder.decodeWifiNan(frame, sizeof(frame), record)));
  TEST_ASSERT_EQUAL_STRING("USS-Enterprise", record.uasId);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_decodes_message_pack_fields);
  RUN_TEST(test_decodes_wifi_beacon_vendor_element);
  RUN_TEST(test_decodes_ble_service_data);
  RUN_TEST(test_rejects_ble_service_data_without_open_drone_id_app_code);
  RUN_TEST(test_rejects_truncated_pack_and_ie);
  RUN_TEST(test_all_truncated_message_lengths_are_rejected);
  RUN_TEST(test_rejects_out_of_range_location_for_plotting);
  RUN_TEST(test_decodes_authentication_and_location_sentinels);
  RUN_TEST(test_decodes_wifi_nan_action_frame);
  return UNITY_END();
}
