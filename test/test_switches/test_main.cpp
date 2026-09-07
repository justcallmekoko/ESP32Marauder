#include <unity.h>

#include "Arduino.h"
#include "Switches.h"

void setUp() { arduino_test::reset(); }
void tearDown() {}

void test_constructor_configures_pullup_and_accessors() {
  Switches button(7, 1000, true);
  TEST_ASSERT_EQUAL_INT(7, button.getPin());
  TEST_ASSERT_TRUE(button.getPullup());
  TEST_ASSERT_EQUAL_INT(7, arduino_test::lastPinModePin());
  TEST_ASSERT_EQUAL_INT(INPUT_PULLUP, arduino_test::lastPinModeValue());
  TEST_ASSERT_FALSE(button.isHeld());
}

void test_constructor_configures_pulldown() {
  Switches button(9, 500, false);
  TEST_ASSERT_FALSE(button.getPullup());
  TEST_ASSERT_EQUAL_INT(INPUT_PULLDOWN, arduino_test::lastPinModeValue());
}

void test_default_constructor_starts_released() {
  Switches button;
  TEST_ASSERT_EQUAL_INT(0, button.getPin());
  TEST_ASSERT_EQUAL_INT(0, arduino_test::lastPinModePin());
  TEST_ASSERT_EQUAL_INT(INPUT, arduino_test::lastPinModeValue());
  TEST_ASSERT_FALSE(button.isHeld());
}

void test_pullup_press_hold_and_release_sequence() {
  Switches button(7, 1000, true);
  arduino_test::setDigitalRead(LOW);
  arduino_test::setMillis(100);
  TEST_ASSERT_TRUE(button.justPressed());

  arduino_test::setMillis(1099);
  TEST_ASSERT_FALSE(button.justPressed());
  TEST_ASSERT_FALSE(button.isHeld());

  arduino_test::setMillis(1100);
  TEST_ASSERT_FALSE(button.justPressed());
  TEST_ASSERT_TRUE(button.isHeld());

  arduino_test::setDigitalRead(HIGH);
  TEST_ASSERT_TRUE(button.justReleased());
  TEST_ASSERT_FALSE(button.isHeld());
  TEST_ASSERT_FALSE(button.justReleased());
}

void test_pulldown_uses_high_as_pressed() {
  Switches button(9, 500, false);
  arduino_test::setDigitalRead(HIGH);
  TEST_ASSERT_TRUE(button.justPressed());
  arduino_test::setDigitalRead(LOW);
  TEST_ASSERT_TRUE(button.justReleased());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_constructor_configures_pullup_and_accessors);
  RUN_TEST(test_constructor_configures_pulldown);
  RUN_TEST(test_default_constructor_starts_released);
  RUN_TEST(test_pullup_press_hold_and_release_sequence);
  RUN_TEST(test_pulldown_uses_high_as_pressed);
  return UNITY_END();
}
