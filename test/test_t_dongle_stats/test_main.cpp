#include <unity.h>

#include "TDongleStats.h"

void test_mode_labels_cover_retained_scan_views() {
  TEST_ASSERT_EQUAL_STRING("IDLE", TDongleStats::modeLabel(0));
  TEST_ASSERT_EQUAL_STRING("WIFI AP", TDongleStats::modeLabel(2));
  TEST_ASSERT_EQUAL_STRING("WIFI STA", TDongleStats::modeLabel(26));
  TEST_ASSERT_EQUAL_STRING("AP+STA", TDongleStats::modeLabel(49));
  TEST_ASSERT_EQUAL_STRING("WIFI ALL", TDongleStats::modeLabel(6));
  TEST_ASSERT_EQUAL_STRING("WARDRIVE", TDongleStats::modeLabel(32));
  TEST_ASSERT_EQUAL_STRING("BLE ALL", TDongleStats::modeLabel(10));
  TEST_ASSERT_EQUAL_STRING("BLE DRIVE", TDongleStats::modeLabel(34));
  TEST_ASSERT_EQUAL_STRING("BLE DRIVE", TDongleStats::modeLabel(35));
  TEST_ASSERT_EQUAL_STRING("ACTIVE", TDongleStats::modeLabel(255));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_mode_labels_cover_retained_scan_views);
  return UNITY_END();
}
