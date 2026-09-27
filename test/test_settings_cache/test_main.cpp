#include <unity.h>

#include "SettingsCache.h"

// Mirror of configs.h JSON_SETTING_SIZE (the budget Settings uses at runtime).
static const size_t kJsonCapacity = 6144;

static const char* kFullSettings =
    "{\"Settings\":["
    "{\"name\":\"ForcePMKID\",\"value\":true},"
    "{\"name\":\"SavePCAP\",\"value\":false},"
    "{\"name\":\"ChanHop\",\"value\":true},"
    "{\"name\":\"ClientSSID\",\"value\":\"lab-ap\"},"
    "{\"name\":\"ClientPW\",\"value\":\"hunter2\"}"
    "]}";

// 1. Every recognised key lands in the matching field.
void test_build_populates_all_known_fields() {
  SettingsCache c;
  TEST_ASSERT_TRUE(buildSettingsCache(kFullSettings, c, kJsonCapacity));
  TEST_ASSERT_TRUE(c.ForcePMKID);
  TEST_ASSERT_FALSE(c.SavePCAP);
  TEST_ASSERT_TRUE(c.ChanHop);
  TEST_ASSERT_EQUAL_STRING("lab-ap", c.ClientSSID.c_str());
  TEST_ASSERT_EQUAL_STRING("hunter2", c.ClientPW.c_str());
}

// 2. Keys absent from JSON keep their struct defaults (SavePCAP/EnableLED = true).
void test_defaults_survive_missing_keys() {
  SettingsCache c;
  TEST_ASSERT_TRUE(buildSettingsCache("{\"Settings\":[{\"name\":\"ChanHop\",\"value\":true}]}", c, kJsonCapacity));
  TEST_ASSERT_TRUE(c.SavePCAP);
  TEST_ASSERT_TRUE(c.EnableLED);
  TEST_ASSERT_TRUE(c.ChanHop);
}

// 3. Invalid JSON returns false and leaves the cache untouched.
void test_invalid_json_leaves_cache_untouched() {
  SettingsCache c;
  c.ChanHop = true;
  TEST_ASSERT_FALSE(buildSettingsCache("}{ not json", c, kJsonCapacity));
  TEST_ASSERT_TRUE(c.SavePCAP);   // default preserved
  TEST_ASSERT_TRUE(c.ChanHop);    // prior value preserved
}

// 4. Bool coerces from JSON bool and from the string form "true"/numeric 0.
void test_bool_values_parse_from_multiple_forms() {
  SettingsCache c;
  TEST_ASSERT_TRUE(buildSettingsCache("{\"Settings\":[{\"name\":\"ChanHop\",\"value\":\"true\"}]}", c, kJsonCapacity));
  TEST_ASSERT_TRUE(c.ChanHop);
  SettingsCache c2;
  c2.EnableLED = true;
  TEST_ASSERT_TRUE(buildSettingsCache("{\"Settings\":[{\"name\":\"EnableLED\",\"value\":0}]}", c2, kJsonCapacity));
  TEST_ASSERT_FALSE(c2.EnableLED);
}

// 5. The save-sync path matches a full rebuild (the invariant the cache risks).
void test_save_sync_matches_rebuild() {
  SettingsCache synced;
  TEST_ASSERT_TRUE(applyCachedBool(synced, "ChanHop", true));
  TEST_ASSERT_TRUE(applyCachedString(synced, "ClientSSID", String("lab-ap")));

  SettingsCache rebuilt;
  TEST_ASSERT_TRUE(buildSettingsCache(
      "{\"Settings\":[{\"name\":\"ChanHop\",\"value\":true},"
      "{\"name\":\"ClientSSID\",\"value\":\"lab-ap\"}]}", rebuilt, kJsonCapacity));

  TEST_ASSERT_EQUAL(rebuilt.ChanHop, synced.ChanHop);
  TEST_ASSERT_EQUAL_STRING(rebuilt.ClientSSID.c_str(), synced.ClientSSID.c_str());
}

// 6. Unknown keys are a no-op on the sync helpers (caller keeps its fallback).
void test_unknown_key_is_noop() {
  SettingsCache c;
  TEST_ASSERT_FALSE(applyCachedBool(c, "Nonexistent", true));
  TEST_ASSERT_FALSE(applyCachedString(c, "Nonexistent", String("x")));
}

// 7. String fields round-trip empty and non-ASCII values without truncation.
void test_string_fields_roundtrip_empty_and_utf() {
  SettingsCache c;
  TEST_ASSERT_TRUE(buildSettingsCache(
      "{\"Settings\":[{\"name\":\"ClientPW\",\"value\":\"\"},"
      "{\"name\":\"ClientSSID\",\"value\":\"sie\xC4\x87-\xC5\xBB\"}]}", c, kJsonCapacity));
  TEST_ASSERT_EQUAL_STRING("", c.ClientPW.c_str());
  TEST_ASSERT_EQUAL_STRING("sie\xC4\x87-\xC5\xBB", c.ClientSSID.c_str());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_build_populates_all_known_fields);
  RUN_TEST(test_defaults_survive_missing_keys);
  RUN_TEST(test_invalid_json_leaves_cache_untouched);
  RUN_TEST(test_bool_values_parse_from_multiple_forms);
  RUN_TEST(test_save_sync_matches_rebuild);
  RUN_TEST(test_unknown_key_is_noop);
  RUN_TEST(test_string_fields_roundtrip_empty_and_utf);
  return UNITY_END();
}
