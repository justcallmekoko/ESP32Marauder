#include <unity.h>

#include "MemoryGuard.h"

void setUp() {}
void tearDown() {}

void test_allows_allocations_with_healthy_heap() {
  marauder::MemoryGuardState guard;
  TEST_ASSERT_TRUE(guard.allow(100 * 1024, 50 * 1024, 1024));
  TEST_ASSERT_FALSE(guard.paused());
}

void test_pauses_before_reserve_is_consumed() {
  marauder::MemoryGuardState guard;
  TEST_ASSERT_FALSE(guard.allow(25 * 1024, 20 * 1024, 2 * 1024));
  TEST_ASSERT_TRUE(guard.paused());
}

void test_fragmentation_also_triggers_pressure() {
  marauder::MemoryGuardState guard;
  TEST_ASSERT_FALSE(guard.allow(100 * 1024, 9 * 1024, 2 * 1024));
  TEST_ASSERT_TRUE(guard.paused());
}

void test_hysteresis_requires_full_recovery() {
  marauder::MemoryGuardState guard;
  TEST_ASSERT_FALSE(guard.allow(20 * 1024, 7 * 1024));
  TEST_ASSERT_FALSE(guard.allow(39 * 1024, 20 * 1024));
  TEST_ASSERT_FALSE(guard.allow(50 * 1024, 11 * 1024));
  TEST_ASSERT_TRUE(guard.allow(50 * 1024, 20 * 1024));
  TEST_ASSERT_FALSE(guard.paused());
}

void test_queue_backpressure_waits_until_half_drained() {
  marauder::QueueBackpressureState queue(20);
  TEST_ASSERT_TRUE(queue.allow(19));
  TEST_ASSERT_FALSE(queue.allow(20));
  TEST_ASSERT_TRUE(queue.paused());
  TEST_ASSERT_FALSE(queue.allow(11));
  TEST_ASSERT_TRUE(queue.allow(10));
  TEST_ASSERT_FALSE(queue.paused());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_allows_allocations_with_healthy_heap);
  RUN_TEST(test_pauses_before_reserve_is_consumed);
  RUN_TEST(test_fragmentation_also_triggers_pressure);
  RUN_TEST(test_hysteresis_requires_full_recovery);
  RUN_TEST(test_queue_backpressure_waits_until_half_drained);
  return UNITY_END();
}
