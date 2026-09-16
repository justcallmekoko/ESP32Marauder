#include <unity.h>

#include "WdgResponse.h"

void setUp() {}
void tearDown() {}

void test_extracts_duplicate_detail_as_short_reason() {
  char reason[64];
  TEST_ASSERT_TRUE(extractWdgErrorReason(
    "HTTP/1.1 409 Conflict\r\nContent-Type: application/json\r\n\r\n"
    "{\"detail\":\"This file is a duplicate upload\"}",
    reason, sizeof(reason)));
  TEST_ASSERT_EQUAL_STRING("Duplicate upload", reason);
}

void test_extracts_message_and_collapses_whitespace() {
  char reason[64];
  TEST_ASSERT_TRUE(extractWdgErrorReason(
    "HTTP/1.1 400 Bad Request\r\n\r\n{\"message\":\"Invalid   CSV\\nformat\"}",
    reason, sizeof(reason)));
  TEST_ASSERT_EQUAL_STRING("Invalid CSV format", reason);
}

void test_supports_error_and_reason_fields() {
  char reason[64];
  TEST_ASSERT_TRUE(extractWdgErrorReason("{\"error\":\"Missing GPS data\"}", reason,
                                        sizeof(reason)));
  TEST_ASSERT_EQUAL_STRING("Missing GPS data", reason);
  TEST_ASSERT_TRUE(extractWdgErrorReason("{\"reason\":\"API key rejected\"}", reason,
                                        sizeof(reason)));
  TEST_ASSERT_EQUAL_STRING("API key rejected", reason);
}

void test_falls_back_to_plain_body_or_http_status() {
  char reason[64];
  TEST_ASSERT_TRUE(extractWdgErrorReason("HTTP/1.1 422 Unprocessable Entity\r\n\r\nBad CSV",
                                        reason, sizeof(reason)));
  TEST_ASSERT_EQUAL_STRING("Bad CSV", reason);
  TEST_ASSERT_TRUE(extractWdgErrorReason("HTTP/1.1 503 Service Unavailable\r\n\r\n",
                                        reason, sizeof(reason)));
  TEST_ASSERT_EQUAL_STRING("503 Service Unavailable", reason);
}

void test_rejects_empty_responses_and_respects_output_size() {
  char reason[8];
  TEST_ASSERT_FALSE(extractWdgErrorReason("", reason, sizeof(reason)));
  TEST_ASSERT_TRUE(extractWdgErrorReason("{\"detail\":\"Long server reason\"}", reason,
                                        sizeof(reason)));
  TEST_ASSERT_EQUAL_STRING("Long se", reason);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_extracts_duplicate_detail_as_short_reason);
  RUN_TEST(test_extracts_message_and_collapses_whitespace);
  RUN_TEST(test_supports_error_and_reason_fields);
  RUN_TEST(test_falls_back_to_plain_body_or_http_status);
  RUN_TEST(test_rejects_empty_responses_and_respects_output_size);
  return UNITY_END();
}
