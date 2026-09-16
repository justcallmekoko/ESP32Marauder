#include <unity.h>

#include "PcapHeader.h"

void setUp() {}
void tearDown() {}

void test_pcap_header_uses_expected_wire_format() {
  uint8_t output[marauder::kPcapGlobalHeaderSize] = {};
  const uint8_t expected[marauder::kPcapGlobalHeaderSize] = {
      0xd4, 0xc3, 0xb2, 0xa1, 0x02, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x10, 0x00, 0x00, 0x69, 0x00, 0x00, 0x00};

  marauder::makePcapGlobalHeader(4096, output);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, output,
                                marauder::kPcapGlobalHeaderSize);
}

void test_pcap_header_serializes_full_snapshot_length() {
  uint8_t output[marauder::kPcapGlobalHeaderSize] = {};

  marauder::makePcapGlobalHeader(0x89abcdef, output);

  const uint8_t expected[] = {0xef, 0xcd, 0xab, 0x89};
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, output + 16, sizeof(expected));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pcap_header_uses_expected_wire_format);
  RUN_TEST(test_pcap_header_serializes_full_snapshot_length);
  return UNITY_END();
}
