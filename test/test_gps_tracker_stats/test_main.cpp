#include <unity.h>

#include "GpsTrackerStats.h"

void setUp() {}
void tearDown() {}

void test_reset_clears_session_and_elapsed_handles_rollover() {
  marauder::GpsTrackerStats stats;
  stats.reset(UINT32_MAX - 10);

  TEST_ASSERT_EQUAL_UINT32(16, stats.elapsedMs(5));
  TEST_ASSERT_EQUAL_UINT32(0, stats.loggedPoints());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, stats.distanceMeters());
}

void test_known_coordinates_produce_expected_distance() {
  const float distance = marauder::GpsTrackerStats::distanceBetweenMeters(
      40748147, -73985364, 40758147, -73985364);
  TEST_ASSERT_FLOAT_WITHIN(2.0f, 1111.95f, distance);
}

void test_first_fix_sets_origin_without_adding_distance() {
  marauder::GpsTrackerStats stats;
  stats.reset(1000);

  TEST_ASSERT_TRUE(stats.update(40748147, -73985364, 2.0f, 2000));
  TEST_ASSERT_TRUE(stats.hasFix());
  TEST_ASSERT_EQUAL_UINT32(1, stats.loggedPoints());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, stats.distanceMeters());
}

void test_real_movement_accumulates_and_reports_speed() {
  marauder::GpsTrackerStats stats;
  stats.reset(0);
  stats.update(40748147, -73985364, 2.0f, 1000);

  TEST_ASSERT_TRUE(stats.update(40748237, -73985364, 2.0f, 2000));
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, stats.distanceMeters());
  TEST_ASSERT_FLOAT_WITHIN(0.5f, 10.0f, stats.speedMetersPerSecond());
  TEST_ASSERT_EQUAL_UINT32(2, stats.loggedPoints());
}

void test_accuracy_jitter_and_impossible_jump_are_not_accumulated() {
  marauder::GpsTrackerStats stats;
  stats.reset(0);
  stats.update(40748147, -73985364, 8.0f, 1000);

  TEST_ASSERT_FALSE(stats.update(40748192, -73985364, 8.0f, 2000));
  TEST_ASSERT_FALSE(stats.update(41748192, -73985364, 2.0f, 3000));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, stats.distanceMeters());
  TEST_ASSERT_EQUAL_UINT32(3, stats.loggedPoints());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_reset_clears_session_and_elapsed_handles_rollover);
  RUN_TEST(test_known_coordinates_produce_expected_distance);
  RUN_TEST(test_first_fix_sets_origin_without_adding_distance);
  RUN_TEST(test_real_movement_accumulates_and_reports_speed);
  RUN_TEST(test_accuracy_jitter_and_impossible_jump_are_not_accumulated);
  return UNITY_END();
}
