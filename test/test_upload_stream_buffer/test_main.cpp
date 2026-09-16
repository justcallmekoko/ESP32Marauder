#include <unity.h>

#include "UploadStreamBuffer.h"

void setUp(void) {}
void tearDown(void) {}

void test_upload_buffer_uses_fixed_small_chunk(void) {
  TEST_ASSERT_EQUAL_UINT32(1024, marauder::UPLOAD_STREAM_BUFFER_SIZE);
  TEST_ASSERT_EQUAL_UINT32(sizeof(void*), sizeof(marauder::UploadStreamBuffer));
}

void test_upload_buffer_allocates_from_heap(void) {
  marauder::UploadStreamBuffer buffer;
  TEST_ASSERT_TRUE(static_cast<bool>(buffer));
  TEST_ASSERT_NOT_NULL(buffer.data());
  TEST_ASSERT_EQUAL_UINT32(marauder::UPLOAD_STREAM_BUFFER_SIZE, buffer.size());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_upload_buffer_uses_fixed_small_chunk);
  RUN_TEST(test_upload_buffer_allocates_from_heap);
  return UNITY_END();
}
