#include <unity.h>

#include "WigleFormat.h"

#include <cstring>

void setUp() {}
void tearDown() {}

void test_wigle_header_has_fourteen_columns() {
  size_t columns = 1;
  for (const char* cursor = WIGLE_CSV_COLUMNS; *cursor; ++cursor)
    if (*cursor == ',') ++columns;
  TEST_ASSERT_EQUAL_UINT32(14, columns);
  TEST_ASSERT_NOT_EQUAL(nullptr, std::strstr(WIGLE_CSV_COLUMNS, "Frequency"));
  TEST_ASSERT_NOT_EQUAL(nullptr, std::strstr(WIGLE_CSV_COLUMNS, "RCOIs,MfgrId,Type"));
}

void test_wigle_channel_frequency_conversion() {
  TEST_ASSERT_EQUAL_UINT16(2412, wigleFrequencyMHz(1));
  TEST_ASSERT_EQUAL_UINT16(2472, wigleFrequencyMHz(13));
  TEST_ASSERT_EQUAL_UINT16(2484, wigleFrequencyMHz(14));
  TEST_ASSERT_EQUAL_UINT16(5180, wigleFrequencyMHz(36));
  TEST_ASSERT_EQUAL_UINT16(5885, wigleFrequencyMHz(177));
  TEST_ASSERT_EQUAL_UINT16(0, wigleFrequencyMHz(0));
  TEST_ASSERT_EQUAL_UINT16(0, wigleFrequencyMHz(200));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_wigle_header_has_fourteen_columns);
  RUN_TEST(test_wigle_channel_frequency_conversion);
  return UNITY_END();
}
