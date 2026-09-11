#include <unity.h>

#include "IPv4Range.h"

namespace {

constexpr uint32_t ip(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  return (static_cast<uint32_t>(a) << 24) |
         (static_cast<uint32_t>(b) << 16) |
         (static_cast<uint32_t>(c) << 8) | static_cast<uint32_t>(d);
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_slash_24_range_uses_network_and_broadcast_boundaries() {
  const auto range = marauder::ipv4HostRange(ip(192, 168, 1, 77),
                                              ip(255, 255, 255, 0));
  TEST_ASSERT_TRUE(range.valid);
  TEST_ASSERT_EQUAL_HEX32(ip(192, 168, 1, 0), range.network);
  TEST_ASSERT_EQUAL_HEX32(ip(192, 168, 1, 255), range.broadcast);
  TEST_ASSERT_EQUAL_HEX32(ip(192, 168, 1, 1), range.first);
  TEST_ASSERT_EQUAL_HEX32(ip(192, 168, 1, 254), range.last);
}

void test_slash_23_range_includes_hosts_below_gateway_octet() {
  const auto range = marauder::ipv4HostRange(ip(10, 0, 5, 1),
                                              ip(255, 255, 254, 0));
  TEST_ASSERT_EQUAL_HEX32(ip(10, 0, 4, 1), range.first);
  TEST_ASSERT_EQUAL_HEX32(ip(10, 0, 5, 254), range.last);
  TEST_ASSERT_EQUAL_HEX32(range.first,
                          marauder::nextIPv4Host(range.network, range));
}

void test_gateway_near_broadcast_does_not_shorten_range() {
  const auto range = marauder::ipv4HostRange(ip(172, 16, 35, 126),
                                              ip(255, 255, 255, 192));
  TEST_ASSERT_EQUAL_HEX32(ip(172, 16, 35, 65), range.first);
  TEST_ASSERT_EQUAL_HEX32(ip(172, 16, 35, 126), range.last);
}

void test_iteration_stops_after_last_host() {
  const auto range = marauder::ipv4HostRange(ip(192, 168, 1, 10),
                                              ip(255, 255, 255, 252));
  TEST_ASSERT_EQUAL_HEX32(ip(192, 168, 1, 9),
                          marauder::nextIPv4Host(range.network, range));
  TEST_ASSERT_EQUAL_HEX32(ip(192, 168, 1, 10),
                          marauder::nextIPv4Host(range.first, range));
  TEST_ASSERT_EQUAL_HEX32(0, marauder::nextIPv4Host(range.last, range));
}

void test_previous_host_rejects_underflow_and_accepts_current_host() {
  const auto range = marauder::ipv4HostRange(ip(192, 168, 1, 10),
                                              ip(255, 255, 255, 0));
  TEST_ASSERT_EQUAL_HEX32(ip(192, 168, 1, 10),
                          marauder::previousIPv4Host(ip(192, 168, 1, 10), 0,
                                                    range));
  TEST_ASSERT_EQUAL_HEX32(ip(192, 168, 1, 1),
                          marauder::previousIPv4Host(ip(192, 168, 1, 10), 9,
                                                    range));
  TEST_ASSERT_EQUAL_HEX32(0,
                          marauder::previousIPv4Host(ip(192, 168, 1, 10), 10,
                                                    range));
}

void test_noncontiguous_and_hostless_masks_are_rejected() {
  TEST_ASSERT_FALSE(
      marauder::ipv4HostRange(ip(10, 0, 0, 1), ip(255, 0, 255, 0)).valid);
  TEST_ASSERT_FALSE(
      marauder::ipv4HostRange(ip(10, 0, 0, 1), ip(255, 255, 255, 254)).valid);
  TEST_ASSERT_FALSE(
      marauder::ipv4HostRange(ip(10, 0, 0, 1), ip(255, 255, 255, 255)).valid);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_slash_24_range_uses_network_and_broadcast_boundaries);
  RUN_TEST(test_slash_23_range_includes_hosts_below_gateway_octet);
  RUN_TEST(test_gateway_near_broadcast_does_not_shorten_range);
  RUN_TEST(test_iteration_stops_after_last_host);
  RUN_TEST(test_previous_host_rejects_underflow_and_accepts_current_host);
  RUN_TEST(test_noncontiguous_and_hostless_masks_are_rejected);
  return UNITY_END();
}
