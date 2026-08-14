#include <unity.h>

#include "DisplayLine.h"

void setUp() {}
void tearDown() {}

void test_short_line_is_padded_to_visible_width() {
  char output[7];
  fitDisplayLine(output, sizeof(output), "AP");
  TEST_ASSERT_EQUAL_STRING("AP    ", output);
}

void test_long_line_is_clipped_to_visible_width() {
  char output[7];
  fitDisplayLine(output, sizeof(output), "123456789");
  TEST_ASSERT_EQUAL_STRING("123456", output);
}

void test_exact_line_is_preserved() {
  char output[7];
  fitDisplayLine(output, sizeof(output), "123456");
  TEST_ASSERT_EQUAL_STRING("123456", output);
}

void test_null_input_produces_blank_line() {
  char output[5];
  fitDisplayLine(output, sizeof(output), nullptr);
  TEST_ASSERT_EQUAL_STRING("    ", output);
}

void test_zero_sized_output_is_ignored() {
  char output = 'X';
  fitDisplayLine(&output, 0, "AP");
  TEST_ASSERT_EQUAL_CHAR('X', output);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_short_line_is_padded_to_visible_width);
  RUN_TEST(test_long_line_is_clipped_to_visible_width);
  RUN_TEST(test_exact_line_is_preserved);
  RUN_TEST(test_null_input_produces_blank_line);
  RUN_TEST(test_zero_sized_output_is_ignored);
  return UNITY_END();
}
