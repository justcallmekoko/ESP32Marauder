#include <unity.h>
#include "Arduino.h"
#include "Switches.h"

void setUp() { arduino_test::reset(); }
void tearDown() {}

void test_bounce_yields_one_press_and_one_release() {
  Switches button(11, 1000, true);
  arduino_test::setDigitalRead(LOW);
  TEST_ASSERT_FALSE(button.justPressed());
  arduino_test::setMillis(10);
  arduino_test::setDigitalRead(HIGH);
  TEST_ASSERT_FALSE(button.justPressed());
  arduino_test::setMillis(15);
  arduino_test::setDigitalRead(LOW);
  TEST_ASSERT_FALSE(button.justPressed());
  arduino_test::setMillis(44);
  TEST_ASSERT_FALSE(button.justPressed());
  arduino_test::setMillis(45);
  TEST_ASSERT_TRUE(button.justPressed());
  TEST_ASSERT_FALSE(button.justPressed());
  arduino_test::setMillis(1045);
  TEST_ASSERT_FALSE(button.justPressed());
  TEST_ASSERT_TRUE(button.isHeld());
  arduino_test::setDigitalRead(HIGH);
  TEST_ASSERT_FALSE(button.justReleased());
  arduino_test::setMillis(1075);
  TEST_ASSERT_TRUE(button.justReleased());
  TEST_ASSERT_FALSE(button.isHeld());
  TEST_ASSERT_FALSE(button.justReleased());
}

void test_debounce_survives_millis_wraparound() {
  Switches button(42, 1000, true);
  arduino_test::setMillis(UINT32_MAX - 10);
  arduino_test::setDigitalRead(LOW);
  TEST_ASSERT_FALSE(button.justPressed());
  arduino_test::setMillis(18);
  TEST_ASSERT_FALSE(button.justPressed());
  arduino_test::setMillis(19);
  TEST_ASSERT_TRUE(button.justPressed());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_bounce_yields_one_press_and_one_release);
  RUN_TEST(test_debounce_survives_millis_wraparound);
  return UNITY_END();
}
