#include <unity.h>

#include "SdStreamProtocol.h"

void setUp() {}
void tearDown() {}

void test_crc32_matches_standard_vector() {
  const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  uint32_t crc = marauder::sdStreamCrc32Update(
      marauder::kSdStreamCrc32Initial, data, sizeof(data));
  TEST_ASSERT_EQUAL_HEX32(0xcbf43926U, marauder::sdStreamCrc32Finish(crc));
}

void test_crc32_is_chunk_boundary_independent() {
  const uint8_t data[] = {0, 1, 2, 3, 4, 5, 0xff};
  uint32_t whole = marauder::sdStreamCrc32Update(
      marauder::kSdStreamCrc32Initial, data, sizeof(data));
  uint32_t split = marauder::sdStreamCrc32Update(
      marauder::kSdStreamCrc32Initial, data, 3);
  split = marauder::sdStreamCrc32Update(split, data + 3, sizeof(data) - 3);
  TEST_ASSERT_EQUAL_HEX32(marauder::sdStreamCrc32Finish(whole),
                          marauder::sdStreamCrc32Finish(split));
}

void test_base64_chunk_has_no_intermediate_padding() {
  TEST_ASSERT_EQUAL_UINT32(0, marauder::kSdStreamChunkSize % 3);
  TEST_ASSERT_EQUAL_UINT32(256, marauder::kSdStreamEncodedSize);
  TEST_ASSERT_EQUAL_UINT32(257, marauder::kSdStreamEncodedBufferSize);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_crc32_matches_standard_vector);
  RUN_TEST(test_crc32_is_chunk_boundary_independent);
  RUN_TEST(test_base64_chunk_has_no_intermediate_padding);
  return UNITY_END();
}
