#include <unity.h>

#include "MenuInputRepeat.h"

void test_press_fires_immediately_then_waits_one_second() {
  MenuInputRepeat repeat;
  TEST_ASSERT_TRUE(repeat.update(true, 100));
  TEST_ASSERT_FALSE(repeat.update(true, 1099));
  TEST_ASSERT_TRUE(repeat.update(true, 1100));
}

void test_held_input_repeats_at_rapid_interval() {
  MenuInputRepeat repeat;
  TEST_ASSERT_TRUE(repeat.update(true, 0));
  TEST_ASSERT_TRUE(repeat.update(true, 1000));
  TEST_ASSERT_FALSE(repeat.update(true, 1119));
  TEST_ASSERT_TRUE(repeat.update(true, 1120));
  TEST_ASSERT_TRUE(repeat.update(true, 1240));
}

void test_release_resets_hold_timing() {
  MenuInputRepeat repeat;
  TEST_ASSERT_TRUE(repeat.update(true, 0));
  TEST_ASSERT_FALSE(repeat.update(false, 500));
  TEST_ASSERT_TRUE(repeat.update(true, 600));
  TEST_ASSERT_FALSE(repeat.update(true, 1599));
  TEST_ASSERT_TRUE(repeat.update(true, 1600));
}

void test_touch_select_release_survives_held_state_polling() {
  constexpr int8_t select_button = 2;
  TEST_ASSERT_EQUAL_INT8(-1,
                         menuTouchReleasedButton(-1, select_button, true));
  TEST_ASSERT_EQUAL_INT8(
      select_button,
      menuTouchReleasedButton(select_button, -1, false));
}

void test_touch_drag_off_does_not_select_before_release() {
  constexpr int8_t select_button = 2;
  TEST_ASSERT_EQUAL_INT8(
      -1,
      menuTouchReleasedButton(select_button, -1, true));
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_press_fires_immediately_then_waits_one_second);
  RUN_TEST(test_held_input_repeats_at_rapid_interval);
  RUN_TEST(test_release_resets_hold_timing);
  RUN_TEST(test_touch_select_release_survives_held_state_polling);
  RUN_TEST(test_touch_drag_off_does_not_select_before_release);
  return UNITY_END();
}
