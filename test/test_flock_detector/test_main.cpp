#include <unity.h>

#include <vector>

#include "FlockDetector.h"

static std::vector<uint8_t> validProbe() {
  std::vector<uint8_t> frame(24, 0);
  frame[0] = 0x40;
  const uint8_t ies[] = {
    0x00, 0x00,
    0x01, 0x02, 0x82, 0x84,
    0xdd, 0x07, 0x50, 0x6f, 0x9a, 0x16, 0x03, 0x01, 0x03,
    0x2d, 0x02, 0x01, 0x02,
    0xbf, 0x01, 0x03,
    0xdd, 0x07, 0x00, 0x50, 0xf2, 0x08, 0x00, 0x00, 0x00
  };
  frame.insert(frame.end(), ies, ies + sizeof(ies));
  return frame;
}

void test_accepts_exact_fingerprint() {
  auto frame = validProbe();
  TEST_ASSERT_TRUE(flockProbeRequestMatches(frame.data(), frame.size()));
}

void test_accepts_optional_fcs() {
  auto frame = validProbe();
  frame.insert(frame.end(), {0xde, 0xad, 0xbe, 0xef});
  TEST_ASSERT_TRUE(flockProbeRequestMatches(frame.data(), frame.size()));
}

void test_requires_probe_request() {
  auto frame = validProbe();
  frame[0] = 0x80;
  TEST_ASSERT_FALSE(flockProbeRequestMatches(frame.data(), frame.size()));
}

void test_requires_wildcard_ssid() {
  auto frame = validProbe();
  frame[25] = 1;
  TEST_ASSERT_FALSE(flockProbeRequestMatches(frame.data(), frame.size()));
}

void test_rejects_changed_vendor_fingerprint() {
  auto frame = validProbe();
  frame[24 + 6 + 4] ^= 0x01;
  TEST_ASSERT_FALSE(flockProbeRequestMatches(frame.data(), frame.size()));
}

void test_rejects_trailing_information_element() {
  auto frame = validProbe();
  frame.insert(frame.end(), {0x01, 0x00});
  TEST_ASSERT_FALSE(flockProbeRequestMatches(frame.data(), frame.size()));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_accepts_exact_fingerprint);
  RUN_TEST(test_accepts_optional_fcs);
  RUN_TEST(test_requires_probe_request);
  RUN_TEST(test_requires_wildcard_ssid);
  RUN_TEST(test_rejects_changed_vendor_fingerprint);
  RUN_TEST(test_rejects_trailing_information_element);
  return UNITY_END();
}

