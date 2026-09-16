#include <unity.h>

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

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_channel_is_written_after_ssid);
  RUN_TEST(test_adjacent_frame_bytes_are_preserved);
  RUN_TEST(test_short_frame_is_rejected);
  RUN_TEST(test_null_frame_is_rejected);
  return UNITY_END();
}
