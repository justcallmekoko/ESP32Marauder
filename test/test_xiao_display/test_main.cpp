#include <unity.h>
#include "Xiao147Display.h"

void setUp() {}
void tearDown() {}

void test_each_rotation_applies_the_board_mirror_and_color_order() {
  Xiao147Display display;
  const uint8_t expected[] = {0x48, 0x28, 0x88, 0xE8};
  for (uint8_t rotation = 0; rotation < 4; ++rotation) {
    display.setRotation(rotation);
    TEST_ASSERT_EQUAL_UINT8(rotation, display.getRotation());
    TEST_ASSERT_EQUAL_HEX8(0x36, display.command);
    TEST_ASSERT_EQUAL_HEX8(expected[rotation], display.data);
  }
}

void test_reinitializing_the_lcd_restores_the_board_correction() {
  Xiao147Display display;
  display.setRotation(2);
  display.init();
  TEST_ASSERT_EQUAL_UINT32(1, display.init_count);
  TEST_ASSERT_EQUAL_UINT8(2, display.getRotation());
  TEST_ASSERT_EQUAL_HEX8(0x88, display.data);
  display.init();
  TEST_ASSERT_EQUAL_UINT32(2, display.init_count);
  TEST_ASSERT_EQUAL_HEX8(0x88, display.data);
}

void test_begin_and_wrapped_rotation_keep_the_correction() {
  Xiao147Display display;
  display.setRotation(5);
  display.begin();
  TEST_ASSERT_EQUAL_UINT32(1, display.init_count);
  TEST_ASSERT_EQUAL_UINT8(1, display.getRotation());
  TEST_ASSERT_EQUAL_HEX8(0x28, display.data);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_each_rotation_applies_the_board_mirror_and_color_order);
  RUN_TEST(test_reinitializing_the_lcd_restores_the_board_correction);
  RUN_TEST(test_begin_and_wrapped_rotation_keep_the_correction);
  return UNITY_END();
}
