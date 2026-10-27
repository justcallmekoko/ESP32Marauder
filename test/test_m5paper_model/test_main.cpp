#include <unity.h>
#include <cstring>
#include "M5PaperModel.h"

static uint8_t a[m5paper::kFrameBytes], b[m5paper::kFrameBytes];
void setUp() { memset(a, 0xff, sizeof(a)); memset(b, 0xff, sizeof(b)); }
void tearDown() {}

void test_touch_and_canvas_agree_at_every_pixel_and_rotation() {
  for (uint8_t rotation = 0; rotation < 4; ++rotation) {
    for (uint16_t y = 0; y < 960; ++y) {
      for (uint16_t x = 0; x < 540; ++x) {
        sticky::Point logical{}, native{};
        TEST_ASSERT_TRUE(m5paper::touchPoint(x, y, 540, 960, rotation, logical));
        TEST_ASSERT_TRUE(m5paper::panelPoint(logical.x, logical.y, rotation, native));
        TEST_ASSERT_EQUAL_UINT16(x, native.x);
        TEST_ASSERT_EQUAL_UINT16(y, native.y);
      }
    }
  }
}
void test_coordinates_reject_out_of_bounds_and_clamp_sensor_endpoint() {
  sticky::Point p{};
  TEST_ASSERT_FALSE(m5paper::touchPoint(541, 0, 540, 960, 0, p));
  TEST_ASSERT_FALSE(m5paper::touchPoint(0, 961, 540, 960, 0, p));
  TEST_ASSERT_FALSE(m5paper::touchPoint(0, 0, 0, 960, 0, p));
  TEST_ASSERT_FALSE(m5paper::touchPoint(0, 0, 540, 0, 0, p));
  TEST_ASSERT_TRUE(m5paper::touchPoint(540, 960, 540, 960, 0, p));
  TEST_ASSERT_EQUAL_UINT16(539, p.x); TEST_ASSERT_EQUAL_UINT16(959, p.y);
  for (uint8_t r = 0; r < 4; ++r) {
    TEST_ASSERT_FALSE(m5paper::panelPoint(-1, 0, r, p));
    TEST_ASSERT_FALSE(m5paper::panelPoint(0, -1, r, p));
    TEST_ASSERT_FALSE(m5paper::panelPoint(r & 1 ? 960 : 540, 0, r, p));
    TEST_ASSERT_FALSE(m5paper::panelPoint(0, r & 1 ? 540 : 960, r, p));
  }
}
void test_padding_bits_do_not_trigger_refresh() {
  for (size_t y = 0; y < 960; ++y) b[y * m5paper::kStride + m5paper::kStride - 1] ^= 0x0f;
  TEST_ASSERT_TRUE(m5paper::difference(a, b).empty());
}
void test_dirty_window_covers_rightmost_pixel_without_exceeding_panel() {
  b[sizeof(b) - 1] ^= 0x10;
  const auto area = m5paper::difference(a,b);
  TEST_ASSERT_EQUAL_UINT16(536, area.x); TEST_ASSERT_EQUAL_UINT16(959, area.y);
  TEST_ASSERT_EQUAL_UINT16(4, area.w); TEST_ASSERT_EQUAL_UINT16(1, area.h);
}
void test_dirty_window_contains_old_and_new_menu_selection() {
  b[150 * m5paper::kStride + 2] = 0;
  b[244 * m5paper::kStride + 3] = 0;
  const auto area = m5paper::difference(a,b);
  TEST_ASSERT_EQUAL_UINT16(16, area.x); TEST_ASSERT_EQUAL_UINT16(150, area.y);
  TEST_ASSERT_EQUAL_UINT16(16, area.w); TEST_ASSERT_EQUAL_UINT16(95, area.h);
  memcpy(a,b,sizeof(a));
  TEST_ASSERT_TRUE(m5paper::difference(a,b).empty());
}
void test_touch_navigation_covers_all_three_full_width_zones() {
  for (uint16_t y = 0; y < 960; ++y) {
    TEST_ASSERT_EQUAL_INT8(y / 320, sticky::menuZone(0, y, 540, 960));
    TEST_ASSERT_EQUAL_INT8(y / 320, sticky::menuZone(539, y, 540, 960));
  }
}
void test_controller_windows_stay_inside_panel_at_every_row() {
  for (uint16_t y = 0; y < 960; ++y) {
    const auto area = m5paper::transferArea({0, y, 8, 1});
    TEST_ASSERT_TRUE(area.y <= y && area.y + area.h > y);
    TEST_ASSERT_TRUE(area.h >= 8 && area.y + area.h <= 960);
    TEST_ASSERT_EQUAL_UINT16(0, area.y % 8);
  }
  TEST_ASSERT_TRUE(m5paper::transferArea({}).empty());
}
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_touch_and_canvas_agree_at_every_pixel_and_rotation);
  RUN_TEST(test_coordinates_reject_out_of_bounds_and_clamp_sensor_endpoint);
  RUN_TEST(test_padding_bits_do_not_trigger_refresh);
  RUN_TEST(test_dirty_window_covers_rightmost_pixel_without_exceeding_panel);
  RUN_TEST(test_dirty_window_contains_old_and_new_menu_selection);
  RUN_TEST(test_touch_navigation_covers_all_three_full_width_zones);
  RUN_TEST(test_controller_windows_stay_inside_panel_at_every_row);
  return UNITY_END();
}
