#include <unity.h>

#include "CensorText.h"

void setUp() {}
void tearDown() {}

void test_names_keep_three_characters() {
  char value[] = "MyHomeWiFi";
  CensorText::name(value);
  TEST_ASSERT_EQUAL_STRING("MyHXXXXXXX", value);
}

void test_macs_keep_two_bytes_and_separators() {
  char value[] = "AA:BB:CC:DD:EE:FF";
  CensorText::mac(value);
  TEST_ASSERT_EQUAL_STRING("AA:BB:XX:XX:XX:XX", value);
}

void test_identifier_selects_mac_or_name() {
  char mac[] = "12-34-56-78-9A-BC";
  char name[] = "Beacon";
  CensorText::identifier(mac);
  CensorText::identifier(name);
  TEST_ASSERT_EQUAL_STRING("12-34-XX-XX-XX-XX", mac);
  TEST_ASSERT_EQUAL_STRING("BeaXXX", name);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_names_keep_three_characters);
  RUN_TEST(test_macs_keep_two_bytes_and_separators);
  RUN_TEST(test_identifier_selects_mac_or_name);
  return UNITY_END();
}
