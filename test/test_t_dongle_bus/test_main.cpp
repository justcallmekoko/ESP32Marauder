#include <unity.h>

#include "Arduino.h"
#include "TDongleBus.h"

void test_shared_spi_devices_are_deselected() {
  arduino_test::reset();

  deselectTDongleSharedSpi(10, 23);

  TEST_ASSERT_EQUAL_INT(2, arduino_test::digitalWriteCount());
  TEST_ASSERT_EQUAL_INT(10, arduino_test::digitalWritePin(0));
  TEST_ASSERT_EQUAL_INT(HIGH, arduino_test::digitalWriteValue(0));
  TEST_ASSERT_EQUAL_INT(23, arduino_test::digitalWritePin(1));
  TEST_ASSERT_EQUAL_INT(HIGH, arduino_test::digitalWriteValue(1));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_shared_spi_devices_are_deselected);
  return UNITY_END();
}
