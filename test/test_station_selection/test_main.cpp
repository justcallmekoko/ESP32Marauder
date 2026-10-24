#include <unity.h>

#include "StationSelection.h"

void test_selecting_station_selects_valid_associated_ap() {
  TEST_ASSERT_EQUAL_INT(2, associatedApToSelect(true, 2, 3));
}

void test_unselecting_station_does_not_change_ap_selection() {
  TEST_ASSERT_EQUAL_INT(-1, associatedApToSelect(false, 2, 3));
}

void test_invalid_associated_ap_is_ignored() {
  TEST_ASSERT_EQUAL_INT(-1, associatedApToSelect(true, 3, 3));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_selecting_station_selects_valid_associated_ap);
  RUN_TEST(test_unselecting_station_does_not_change_ap_selection);
  RUN_TEST(test_invalid_associated_ap_is_ignored);
  return UNITY_END();
}
