#include <unity.h>

#include "BootSplash.h"

void setUp() {}
void tearDown() {}

void assertLayoutFits(int16_t width, int16_t height) {
  const marauder::BootSplashLayout layout =
      marauder::bootSplashLayout(width, height);
  TEST_ASSERT_GREATER_OR_EQUAL_INT16(0, layout.title_y);
  TEST_ASSERT_GREATER_OR_EQUAL_INT16(0, layout.logo_x);
  TEST_ASSERT_GREATER_OR_EQUAL_INT16(0, layout.logo_y);
  TEST_ASSERT_LESS_OR_EQUAL_INT16(width, layout.logo_x + layout.logo_width);
  TEST_ASSERT_LESS_THAN_INT16(layout.version_y, layout.logo_y + layout.logo_height);
  TEST_ASSERT_LESS_THAN_INT16(layout.status_y, layout.version_y);
  TEST_ASSERT_LESS_THAN_INT16(height, layout.status_y);
}

void test_layout_fits_mini_v3() {
  const marauder::BootSplashLayout layout =
      marauder::bootSplashLayout(128, 128);
  assertLayoutFits(128, 128);
  TEST_ASSERT_EQUAL_UINT8(1, layout.text_size);
}

void test_layout_fits_v8_and_uses_more_logo_area() {
  const marauder::BootSplashLayout mini =
      marauder::bootSplashLayout(128, 128);
  const marauder::BootSplashLayout v8 =
      marauder::bootSplashLayout(240, 320);
  assertLayoutFits(240, 320);
  TEST_ASSERT_EQUAL_UINT8(1, v8.text_size);
  TEST_ASSERT_GREATER_THAN_INT16(mini.logo_height * 2, v8.logo_height);
}

void test_layout_fits_landscape_and_square_displays() {
  assertLayoutFits(240, 135);
  assertLayoutFits(240, 240);
  assertLayoutFits(320, 240);
  assertLayoutFits(480, 320);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_layout_fits_mini_v3);
  RUN_TEST(test_layout_fits_v8_and_uses_more_logo_area);
  RUN_TEST(test_layout_fits_landscape_and_square_displays);
  return UNITY_END();
}
