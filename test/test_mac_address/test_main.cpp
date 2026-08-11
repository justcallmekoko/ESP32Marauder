#include <unity.h>

#include "MarauderMacAddress.h"

void setUp() {}

void tearDown() {}

void test_format_mac_address_uses_uppercase_and_zero_padding() {
  const uint8_t mac[marauder::kMacAddressSize] = {0x00, 0x01, 0x0A,
                                                  0x10, 0xAB, 0xFF};
  char output[marauder::kMacAddressTextLength + 1] = {};

  TEST_ASSERT_TRUE(marauder::formatMacAddress(mac, output));
  TEST_ASSERT_EQUAL_STRING("00:01:0A:10:AB:FF", output);
}

void test_parse_mac_address_accepts_uppercase_and_lowercase() {
  uint8_t uppercase[marauder::kMacAddressSize] = {};
  uint8_t lowercase[marauder::kMacAddressSize] = {};
  const uint8_t expected[marauder::kMacAddressSize] = {0x00, 0x01, 0x0A,
                                                       0x10, 0xAB, 0xFF};

  TEST_ASSERT_TRUE(
      marauder::parseMacAddress("00:01:0A:10:AB:FF", uppercase));
  TEST_ASSERT_TRUE(
      marauder::parseMacAddress("00:01:0a:10:ab:ff", lowercase));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, uppercase, marauder::kMacAddressSize);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, lowercase, marauder::kMacAddressSize);
}

void test_parse_mac_address_rejects_invalid_length() {
  uint8_t output[marauder::kMacAddressSize] = {};

  TEST_ASSERT_FALSE(marauder::parseMacAddress("00:11:22:33:44", output));
  TEST_ASSERT_FALSE(
      marauder::parseMacAddress("00:11:22:33:44:55:66", output));
}

void test_parse_mac_address_rejects_invalid_separator_and_hex() {
  uint8_t output[marauder::kMacAddressSize] = {};

  TEST_ASSERT_FALSE(
      marauder::parseMacAddress("00-11-22-33-44-55", output));
  TEST_ASSERT_FALSE(
      marauder::parseMacAddress("00:11:22:33:44:GG", output));
}

void test_parse_mac_address_rejects_invalid_hex_in_each_octet() {
  const size_t hex_offsets[] = {0, 1, 3, 4, 6, 7, 9, 10, 12, 13, 15, 16};
  for (size_t index = 0; index < sizeof(hex_offsets) / sizeof(hex_offsets[0]); ++index) {
    char text[] = "00:11:22:33:44:55";
    uint8_t output[marauder::kMacAddressSize] = {};
    text[hex_offsets[index]] = 'Z';
    TEST_ASSERT_FALSE(marauder::parseMacAddress(text, output));
  }
}

void test_parse_mac_address_rejects_invalid_separator_in_each_position() {
  const size_t separator_offsets[] = {2, 5, 8, 11, 14};
  for (size_t index = 0; index < sizeof(separator_offsets) / sizeof(separator_offsets[0]); ++index) {
    char text[] = "00:11:22:33:44:55";
    uint8_t output[marauder::kMacAddressSize] = {};
    text[separator_offsets[index]] = '-';
    TEST_ASSERT_FALSE(marauder::parseMacAddress(text, output));
  }
}

void test_mac_address_boundary_values_round_trip() {
  const uint8_t expected[marauder::kMacAddressSize] = {0x00, 0x0F, 0x10, 0x7F, 0x80, 0xFF};
  uint8_t parsed[marauder::kMacAddressSize] = {};
  char formatted[marauder::kMacAddressTextLength + 1] = {};
  TEST_ASSERT_TRUE(marauder::formatMacAddress(expected, formatted));
  TEST_ASSERT_EQUAL_STRING("00:0F:10:7F:80:FF", formatted);
  TEST_ASSERT_TRUE(marauder::parseMacAddress(formatted, parsed));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, parsed, marauder::kMacAddressSize);
}

void test_parse_failure_does_not_modify_output() {
  uint8_t output[marauder::kMacAddressSize] = {1, 2, 3, 4, 5, 6};
  const uint8_t expected[marauder::kMacAddressSize] = {1, 2, 3, 4, 5, 6};

  TEST_ASSERT_FALSE(
      marauder::parseMacAddress("00:11:22:33:44:GG", output));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, output, marauder::kMacAddressSize);
}

void test_mac_address_round_trip() {
  const uint8_t expected[marauder::kMacAddressSize] = {0xDE, 0xAD, 0xBE,
                                                       0xEF, 0x00, 0x01};
  uint8_t parsed[marauder::kMacAddressSize] = {};
  char formatted[marauder::kMacAddressTextLength + 1] = {};

  TEST_ASSERT_TRUE(marauder::formatMacAddress(expected, formatted));
  TEST_ASSERT_TRUE(marauder::parseMacAddress(formatted, parsed));
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, parsed, marauder::kMacAddressSize);
}

void test_mac_address_helpers_reject_null_buffers() {
  uint8_t mac[marauder::kMacAddressSize] = {};
  char output[marauder::kMacAddressTextLength + 1] = {};

  TEST_ASSERT_FALSE(marauder::formatMacAddress(nullptr, output));
  TEST_ASSERT_FALSE(marauder::formatMacAddress(mac, nullptr));
  TEST_ASSERT_FALSE(marauder::parseMacAddress(nullptr, mac));
  TEST_ASSERT_FALSE(marauder::parseMacAddress("00:11:22:33:44:55", nullptr));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_format_mac_address_uses_uppercase_and_zero_padding);
  RUN_TEST(test_parse_mac_address_accepts_uppercase_and_lowercase);
  RUN_TEST(test_parse_mac_address_rejects_invalid_length);
  RUN_TEST(test_parse_mac_address_rejects_invalid_separator_and_hex);
  RUN_TEST(test_parse_mac_address_rejects_invalid_hex_in_each_octet);
  RUN_TEST(test_parse_mac_address_rejects_invalid_separator_in_each_position);
  RUN_TEST(test_mac_address_boundary_values_round_trip);
  RUN_TEST(test_parse_failure_does_not_modify_output);
  RUN_TEST(test_mac_address_round_trip);
  RUN_TEST(test_mac_address_helpers_reject_null_buffers);
  return UNITY_END();
}
