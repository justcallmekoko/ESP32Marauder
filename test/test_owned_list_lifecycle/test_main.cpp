#include <unity.h>

#include "OwnedListLifecycle.h"

namespace {
class FakeList {
 public:
  FakeList() { instances++; }
  ~FakeList() { instances--; }

  void clear() {
    clear_calls++;
    value = 0;
  }

  int value = 0;

  static int instances;
  static int clear_calls;
};

int FakeList::instances = 0;
int FakeList::clear_calls = 0;
}

void test_reset_allocates_when_list_is_null() {
  FakeList* list = nullptr;

  TEST_ASSERT_NOT_NULL(resetOwnedList(list));
  TEST_ASSERT_NOT_NULL(list);
  TEST_ASSERT_EQUAL_INT(1, FakeList::instances);
  TEST_ASSERT_EQUAL_INT(0, FakeList::clear_calls);

  delete list;
}

void test_reset_clears_and_replaces_existing_list() {
  FakeList* list = new FakeList();
  list->value = 42;

  TEST_ASSERT_NOT_NULL(resetOwnedList(list));
  TEST_ASSERT_EQUAL_INT(1, FakeList::instances);
  TEST_ASSERT_EQUAL_INT(1, FakeList::clear_calls);
  TEST_ASSERT_EQUAL_INT(0, list->value);

  delete list;
}

void setUp() {
  FakeList::instances = 0;
  FakeList::clear_calls = 0;
}

void tearDown() {
  TEST_ASSERT_EQUAL_INT(0, FakeList::instances);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_reset_allocates_when_list_is_null);
  RUN_TEST(test_reset_clears_and_replaces_existing_list);
  return UNITY_END();
}
