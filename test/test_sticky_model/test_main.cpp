#include <unity.h>
#include <cstring>
#include "StickyModel.h"

static uint8_t a[sticky::kFrameBytes], b[sticky::kFrameBytes];
void setUp() {}
void tearDown() {}

void test_seeed_touch_reference_corners() {
  sticky::Point p;
  TEST_ASSERT_TRUE(sticky::touchPoint(0, 0, 800, 480, 0, p));
  TEST_ASSERT_EQUAL_UINT16(0, p.x); TEST_ASSERT_EQUAL_UINT16(799, p.y);
  TEST_ASSERT_TRUE(sticky::touchPoint(800, 480, 800, 480, 0, p));
  TEST_ASSERT_EQUAL_UINT16(479, p.x); TEST_ASSERT_EQUAL_UINT16(0, p.y);
  TEST_ASSERT_TRUE(sticky::touchPoint(400, 240, 800, 480, 0, p));
  TEST_ASSERT_EQUAL_UINT16(240, p.x); TEST_ASSERT_EQUAL_UINT16(400, p.y);
}
void test_touch_range_validation() {
  sticky::Point p;
  TEST_ASSERT_FALSE(sticky::touchPoint(801, 0, 800, 480, 0, p));
  TEST_ASSERT_FALSE(sticky::touchPoint(0, 481, 800, 480, 0, p));
  TEST_ASSERT_FALSE(sticky::touchPoint(0, 0, 0, 480, 0, p));
  TEST_ASSERT_FALSE(sticky::touchPoint(0, 0, 800, 0, 0, p));
}
void test_cyd_menu_zones_cover_full_screen() {
  for (uint16_t y = 0; y < 800; ++y) {
    const int8_t zone = y < 267 ? 0 : y < 534 ? 1 : 2;
    TEST_ASSERT_EQUAL_INT8(zone, sticky::menuZone(0, y, 480, 800));
    TEST_ASSERT_EQUAL_INT8(zone, sticky::menuZone(479, y, 480, 800));
  }
  TEST_ASSERT_EQUAL_INT8(-1, sticky::menuZone(480, 0, 480, 800));
  TEST_ASSERT_EQUAL_INT8(-1, sticky::menuZone(0, 800, 480, 800));
  TEST_ASSERT_EQUAL_INT8(-1, sticky::menuZone(0, 0, 480, 0));
  TEST_ASSERT_EQUAL_INT8(2, sticky::menuZone(799, 479, 800, 480));
}
void test_detected_portrait_sensor_mapping() {
  sticky::Point p;
  TEST_ASSERT_TRUE(sticky::touchPoint(240, 400, 480, 800, 0, p));
  TEST_ASSERT_EQUAL_UINT16(240, p.x); TEST_ASSERT_EQUAL_UINT16(400, p.y);
  TEST_ASSERT_TRUE(sticky::touchPoint(480, 0, 480, 800, 0, p));
  TEST_ASSERT_EQUAL_UINT16(479, p.x); TEST_ASSERT_EQUAL_UINT16(799, p.y);
}
void test_panel_mounting_orientation() {
  // Reference composition: portrait rotation, then panel_cfg.mirror_x=true.
  // Checking an asymmetric point catches a rotation substituted for a mirror.
  sticky::Point p;
  TEST_ASSERT_TRUE(sticky::panelPoint(0, 0, 0, p));
  TEST_ASSERT_EQUAL_UINT16(0, p.x); TEST_ASSERT_EQUAL_UINT16(0, p.y);
  TEST_ASSERT_TRUE(sticky::panelPoint(479, 799, 0, p));
  TEST_ASSERT_EQUAL_UINT16(799, p.x); TEST_ASSERT_EQUAL_UINT16(479, p.y);
  TEST_ASSERT_TRUE(sticky::panelPoint(31, 107, 0, p));
  TEST_ASSERT_EQUAL_UINT16(107, p.x); TEST_ASSERT_EQUAL_UINT16(31, p.y);
}
void test_rotations_match_same_physical_touch() {
  for (uint16_t y = 0; y <= 480; y += 16) {
    for (uint16_t x = 0; x <= 800; x += 16) {
      sticky::Point logical{}, expected{};
      TEST_ASSERT_TRUE(sticky::touchPoint(x, y, 800, 480, 0, logical));
      TEST_ASSERT_TRUE(sticky::panelPoint(logical.x, logical.y, 0, expected));
      for (uint8_t r = 1; r < 4; ++r) {
        sticky::Point actual{};
        TEST_ASSERT_TRUE(sticky::touchPoint(x, y, 800, 480, r, logical));
        TEST_ASSERT_TRUE(sticky::panelPoint(logical.x, logical.y, r, actual));
        TEST_ASSERT_EQUAL_UINT16(expected.x, actual.x);
        TEST_ASSERT_EQUAL_UINT16(expected.y, actual.y);
      }
    }
  }
}
void test_every_display_pixel_maps_once_in_all_rotations() {
  for (uint8_t r = 0; r < 4; ++r) {
    memset(a, 0, sizeof(a));
    const int16_t w = (r & 1) ? 800 : 480, h = (r & 1) ? 480 : 800;
    for (int16_t y = 0; y < h; ++y) for (int16_t x = 0; x < w; ++x) {
      sticky::Point p;
      TEST_ASSERT_TRUE(sticky::panelPoint(x, y, r, p));
      const size_t i = p.y * sticky::kStride + p.x / 8;
      TEST_ASSERT_LESS_THAN_UINT32(sizeof(a), i);
      const uint8_t mask = 1u << (p.x & 7);
      TEST_ASSERT_EQUAL_UINT8(0, a[i] & mask);
      a[i] |= mask;
    }
    for (uint8_t byte : a) TEST_ASSERT_EQUAL_HEX8(0xff, byte);
    sticky::Point p;
    TEST_ASSERT_FALSE(sticky::panelPoint(-1, 0, r, p));
    TEST_ASSERT_FALSE(sticky::panelPoint(w, 0, r, p));
    TEST_ASSERT_FALSE(sticky::panelPoint(0, h, r, p));
  }
}
void test_dirty_window_is_minimal_and_byte_aligned() {
  memset(a, 0xff, sizeof(a)); memset(b, 0xff, sizeof(b));
  TEST_ASSERT_TRUE(sticky::difference(a, b).empty());
  a[2 * sticky::kStride + 1] = 0;
  a[6 * sticky::kStride + 3] = 0;
  const sticky::Area area = sticky::difference(a, b);
  TEST_ASSERT_EQUAL_UINT16(8, area.x); TEST_ASSERT_EQUAL_UINT16(2, area.y);
  TEST_ASSERT_EQUAL_UINT16(24, area.w); TEST_ASSERT_EQUAL_UINT16(5, area.h);
  a[0] = 0; a[sizeof(a) - 1] = 0;
  const sticky::Area full = sticky::difference(a, b);
  TEST_ASSERT_EQUAL_UINT16(800, full.w); TEST_ASSERT_EQUAL_UINT16(480, full.h);
}
void test_no_ready_report_preserves_press_until_real_release() {
  sticky::TouchState s;
  TEST_ASSERT_TRUE(s.apply(true, 0x81, true, {50, 70}, 10));
  TEST_ASSERT_FALSE(s.apply(true, 0x00, false, {}, 20));
  TEST_ASSERT_TRUE(s.down); TEST_ASSERT_EQUAL_UINT16(50, s.point.x);
  TEST_ASSERT_TRUE(s.apply(true, 0x80, false, {}, 30));
  TEST_ASSERT_FALSE(s.down);
}
void test_bad_reports_and_i2c_failure_do_not_latch_press() {
  sticky::TouchState s;
  s.apply(true, 0x81, true, {50, 70}, 10);
  TEST_ASSERT_FALSE(s.apply(false, 0, false, {}, 259));
  TEST_ASSERT_TRUE(s.apply(false, 0, false, {}, 260));
  TEST_ASSERT_FALSE(s.down);
  s.apply(true, 0x81, true, {}, 300);
  TEST_ASSERT_TRUE(s.apply(true, 0x86, true, {}, 310));
  TEST_ASSERT_FALSE(s.down);
  s.apply(true, 0x81, true, {}, 320);
  TEST_ASSERT_TRUE(s.apply(true, 0x81, false, {}, 330));
}
void test_touch_error_timeout_survives_millis_wrap() {
  sticky::TouchState s;
  s.apply(true, 0x81, true, {}, 0xfffffff0u);
  TEST_ASSERT_FALSE(s.apply(false, 0, false, {}, 0x20u));
  TEST_ASSERT_TRUE(s.apply(false, 0, false, {}, 0x100u));
}
void test_button_debounce_rejects_contact_bounce() {
  sticky::DebouncedButton button;
  TEST_ASSERT_FALSE(button.update(false, 0));
  TEST_ASSERT_FALSE(button.update(true, 100));
  TEST_ASSERT_FALSE(button.update(false, 108));
  TEST_ASSERT_FALSE(button.update(true, 112));
  TEST_ASSERT_FALSE(button.update(true, 136));
  TEST_ASSERT_TRUE(button.update(true, 137));
  TEST_ASSERT_TRUE(button.down);
  TEST_ASSERT_FALSE(button.update(false, 200));
  TEST_ASSERT_FALSE(button.update(true, 208));
  TEST_ASSERT_TRUE(button.down);
  TEST_ASSERT_FALSE(button.update(false, 220));
  TEST_ASSERT_TRUE(button.update(false, 245));
  TEST_ASSERT_FALSE(button.down);
}
void test_power_short_press_selects_only_on_release() {
  sticky::PowerButton button;
  button.update(false, 0); button.update(false, 25);
  TEST_ASSERT_EQUAL_INT(0, int(button.update(true, 100)));
  TEST_ASSERT_EQUAL_INT(0, int(button.update(true, 125)));
  TEST_ASSERT_EQUAL_INT(0, int(button.update(false, 250)));
  TEST_ASSERT_EQUAL_INT(int(sticky::PowerEvent::Select), int(button.update(false, 275)));
  TEST_ASSERT_EQUAL_INT(0, int(button.update(false, 300)));
}
void test_power_hold_shuts_down_once_without_selecting() {
  sticky::PowerButton button;
  button.update(false, 0); button.update(false, 25);
  button.update(true, 100); button.update(true, 125);
  TEST_ASSERT_EQUAL_INT(0, int(button.update(true, 3124)));
  TEST_ASSERT_EQUAL_INT(int(sticky::PowerEvent::Shutdown), int(button.update(true, 3125)));
  TEST_ASSERT_EQUAL_INT(0, int(button.update(true, 5000)));
  button.update(false, 5100);
  TEST_ASSERT_EQUAL_INT(0, int(button.update(false, 5125)));
}
void test_power_boot_hold_is_ignored_until_release() {
  sticky::PowerButton button;
  button.update(true, 0); button.update(true, 25);
  TEST_ASSERT_EQUAL_INT(0, int(button.update(true, 6000)));
  button.update(false, 6100);
  TEST_ASSERT_EQUAL_INT(0, int(button.update(false, 6125)));
  button.update(true, 7000); button.update(true, 7025);
  button.update(false, 7100);
  TEST_ASSERT_EQUAL_INT(int(sticky::PowerEvent::Select), int(button.update(false, 7125)));
}
void test_power_hold_survives_millis_wrap_and_delayed_poll() {
  sticky::PowerButton button;
  button.update(false, 0xfffff000u); button.update(false, 0xfffff020u);
  button.update(true, 0xffffff00u); button.update(true, 0xffffff20u);
  TEST_ASSERT_EQUAL_INT(int(sticky::PowerEvent::Shutdown), int(button.update(false, 0xc00u)));
  TEST_ASSERT_EQUAL_INT(0, int(button.update(false, 0xc20u)));
}
int main() {
  UNITY_BEGIN();
  RUN_TEST(test_seeed_touch_reference_corners);
  RUN_TEST(test_touch_range_validation);
  RUN_TEST(test_cyd_menu_zones_cover_full_screen);
  RUN_TEST(test_detected_portrait_sensor_mapping);
  RUN_TEST(test_panel_mounting_orientation);
  RUN_TEST(test_rotations_match_same_physical_touch);
  RUN_TEST(test_every_display_pixel_maps_once_in_all_rotations);
  RUN_TEST(test_dirty_window_is_minimal_and_byte_aligned);
  RUN_TEST(test_no_ready_report_preserves_press_until_real_release);
  RUN_TEST(test_bad_reports_and_i2c_failure_do_not_latch_press);
  RUN_TEST(test_touch_error_timeout_survives_millis_wrap);
  RUN_TEST(test_button_debounce_rejects_contact_bounce);
  RUN_TEST(test_power_short_press_selects_only_on_release);
  RUN_TEST(test_power_hold_shuts_down_once_without_selecting);
  RUN_TEST(test_power_boot_hold_is_ignored_until_release);
  RUN_TEST(test_power_hold_survives_millis_wrap_and_delayed_poll);
  return UNITY_END();
}
