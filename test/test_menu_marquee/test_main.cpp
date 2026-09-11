#include <unity.h>

#include "MenuMarquee.h"

void test_does_not_scroll_before_or_at_one_second() {
  TEST_ASSERT_EQUAL_UINT16(0, MenuMarquee::offsetForElapsed(0, 12));
  TEST_ASSERT_EQUAL_UINT16(0, MenuMarquee::offsetForElapsed(999, 12));
  TEST_ASSERT_EQUAL_UINT16(0, MenuMarquee::offsetForElapsed(1000, 12));
}

void test_scrolls_after_one_second_and_advances_at_fixed_interval() {
  TEST_ASSERT_EQUAL_UINT16(1, MenuMarquee::offsetForElapsed(1001, 12));
  TEST_ASSERT_EQUAL_UINT16(1, MenuMarquee::offsetForElapsed(1150, 12));
  TEST_ASSERT_EQUAL_UINT16(2, MenuMarquee::offsetForElapsed(1151, 12));
}

void test_does_not_scroll_when_label_fits() {
  TEST_ASSERT_EQUAL_UINT16(0, MenuMarquee::offsetForElapsed(5000, 0));
}

void test_pauses_at_end_then_restarts() {
  const uint16_t max_offset = 3;

  TEST_ASSERT_EQUAL_UINT16(3, MenuMarquee::offsetForElapsed(1301, max_offset));
  TEST_ASSERT_EQUAL_UINT16(3, MenuMarquee::offsetForElapsed(1901, max_offset));
  TEST_ASSERT_EQUAL_UINT16(0, MenuMarquee::offsetForElapsed(2201, max_offset));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_does_not_scroll_before_or_at_one_second);
  RUN_TEST(test_scrolls_after_one_second_and_advances_at_fixed_interval);
  RUN_TEST(test_does_not_scroll_when_label_fits);
  RUN_TEST(test_pauses_at_end_then_restarts);
  return UNITY_END();
}
