#include <unity.h>
#include "GeofenceMath.h"

void test_zero_distance_is_inside() {
  TEST_ASSERT_TRUE(GeofenceMath::distanceMiles(0, 0, 0, 0) < 0.000001);
}

void test_known_distance_is_reasonable() {
  const double miles = GeofenceMath::distanceMiles(40.7128, -74.0060, 40.730610, -73.935242);
  TEST_ASSERT_TRUE(miles > 3.85 && miles < 3.97);
}

void test_antimeridian_distance_uses_short_path() {
  const double miles = GeofenceMath::distanceMiles(0, 179.999, 0, -179.999);
  TEST_ASSERT_TRUE(miles < 0.2);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_zero_distance_is_inside);
  RUN_TEST(test_known_distance_is_reasonable);
  RUN_TEST(test_antimeridian_distance_uses_short_path);
  return UNITY_END();
}
