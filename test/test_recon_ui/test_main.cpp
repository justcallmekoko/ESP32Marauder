#include <unity.h>

#include "ReconUi.h"

void setUp() {}
void tearDown() {}

void test_layout_profiles_cover_supported_display_shapes() {
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ReconLayout::COMPACT_SQUARE),
                        static_cast<int>(reconLayoutFor(128, 128)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ReconLayout::COMPACT_LANDSCAPE),
                        static_cast<int>(reconLayoutFor(240, 135)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ReconLayout::NARROW_PORTRAIT),
                        static_cast<int>(reconLayoutFor(135, 240)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ReconLayout::LARGE),
                        static_cast<int>(reconLayoutFor(240, 320)));
  TEST_ASSERT_EQUAL_UINT8(3, reconRelationshipRows(ReconLayout::LARGE));
  TEST_ASSERT_EQUAL_UINT8(0, reconRelationshipRows(ReconLayout::COMPACT_SQUARE));
}

void test_signal_segments_and_labels_are_monotonic() {
  TEST_ASSERT_EQUAL_UINT8(0, reconSignalSegments(-128));
  TEST_ASSERT_EQUAL_UINT8(1, reconSignalSegments(-95));
  TEST_ASSERT_EQUAL_UINT8(3, reconSignalSegments(-67));
  TEST_ASSERT_EQUAL_UINT8(5, reconSignalSegments(-40));
  TEST_ASSERT_EQUAL_STRING("FAR", reconProximityLabel(-90));
  TEST_ASSERT_EQUAL_STRING("MID", reconProximityLabel(-72));
  TEST_ASSERT_EQUAL_STRING("NEAR", reconProximityLabel(-50));
}

void test_rssi_plot_mapping_clamps_and_increases() {
  TEST_ASSERT_EQUAL_UINT8(0, reconRssiPlotLevel(-128));
  TEST_ASSERT_EQUAL_UINT8(1, reconRssiPlotLevel(-100));
  TEST_ASSERT_TRUE(reconRssiPlotLevel(-80) < reconRssiPlotLevel(-60));
  TEST_ASSERT_EQUAL_UINT8(100, reconRssiPlotLevel(-35));
  TEST_ASSERT_EQUAL_UINT8(100, reconRssiPlotLevel(-10));
}

void test_signal_trend_uses_meaningful_change_threshold() {
  const int8_t approaching[] = {-82, -78, -70};
  const int8_t departing[] = {-55, -60, -66};
  const int8_t steady[] = {-70, -68, -71};
  const int8_t with_gaps[] = {-128, -82, -75, -128};
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ReconSignalTrend::APPROACHING),
                        static_cast<int>(reconSignalTrend(approaching, 3)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ReconSignalTrend::DEPARTING),
                        static_cast<int>(reconSignalTrend(departing, 3)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ReconSignalTrend::STEADY),
                        static_cast<int>(reconSignalTrend(steady, 3)));
  TEST_ASSERT_EQUAL_INT(static_cast<int>(ReconSignalTrend::APPROACHING),
                        static_cast<int>(reconSignalTrend(with_gaps, 4)));
}

void test_decay_is_wrap_safe_and_ignores_unknown_timestamps() {
  TEST_ASSERT_FALSE(reconDeviceExpired(500000, 0));
  TEST_ASSERT_FALSE(reconDeviceExpired(319999, 200000));
  TEST_ASSERT_TRUE(reconDeviceExpired(320000, 200000));
  TEST_ASSERT_TRUE(reconDeviceExpired(500000, 200000));
  TEST_ASSERT_TRUE(reconDeviceExpired(0x00000020, 0xFFF00000, 100000));
}

void test_channel_pages_cycle_and_include_partial_final_page() {
  TEST_ASSERT_EQUAL_UINT8(0, reconChannelPage(0, 51));
  TEST_ASSERT_EQUAL_UINT8(1, reconChannelPage(RECON_CHANNEL_PAGE_MS, 51));
  TEST_ASSERT_EQUAL_UINT8(6, reconChannelPage(RECON_CHANNEL_PAGE_MS * 6, 51));
  TEST_ASSERT_EQUAL_UINT8(0, reconChannelPage(RECON_CHANNEL_PAGE_MS * 7, 51));
  TEST_ASSERT_EQUAL_UINT8(8, reconChannelsOnPage(0, 51));
  TEST_ASSERT_EQUAL_UINT8(3, reconChannelsOnPage(6, 51));
  TEST_ASSERT_EQUAL_UINT8(0, reconChannelsOnPage(7, 51));
}

void test_churn_height_scales_and_clamps() {
  TEST_ASSERT_EQUAL_UINT8(0, reconChurnHeight(0, 10, 20));
  TEST_ASSERT_EQUAL_UINT8(10, reconChurnHeight(5, 10, 20));
  TEST_ASSERT_EQUAL_UINT8(20, reconChurnHeight(10, 10, 20));
  TEST_ASSERT_EQUAL_UINT8(20, reconChurnHeight(20, 10, 20));
}

void test_truncation_preserves_both_ends() {
  char output[10];
  reconTruncate("MarauderNetwork", output, sizeof(output));
  TEST_ASSERT_EQUAL_STRING("Mar..work", output);
  reconTruncate("short", output, sizeof(output));
  TEST_ASSERT_EQUAL_STRING("short", output);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_layout_profiles_cover_supported_display_shapes);
  RUN_TEST(test_signal_segments_and_labels_are_monotonic);
  RUN_TEST(test_rssi_plot_mapping_clamps_and_increases);
  RUN_TEST(test_signal_trend_uses_meaningful_change_threshold);
  RUN_TEST(test_decay_is_wrap_safe_and_ignores_unknown_timestamps);
  RUN_TEST(test_channel_pages_cycle_and_include_partial_final_page);
  RUN_TEST(test_churn_height_scales_and_clamps);
  RUN_TEST(test_truncation_preserves_both_ends);
  return UNITY_END();
}
