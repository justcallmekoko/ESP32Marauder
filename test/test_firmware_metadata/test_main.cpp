#include <unity.h>

#include <cstring>
#include <vector>

#include "FirmwareMetadata.h"

using MarauderFirmware::Metadata;
using MarauderFirmware::MetadataScanner;

namespace {

Metadata makeMetadata(const char* hardware, const char* chip) {
  Metadata metadata = {
    {'M', 'R', 'D', 'R', 'F', 'W', 'I', 'D'},
    MarauderFirmware::METADATA_SCHEMA_VERSION,
    {0, 0, 0},
    {},
    {},
    {'D', 'I', 'W', 'F', 'R', 'D', 'R', 'M'}
  };
  strncpy(metadata.hardware, hardware, sizeof(metadata.hardware) - 1);
  strncpy(metadata.chip, chip, sizeof(metadata.chip) - 1);
  return metadata;
}

Metadata scan(const std::vector<uint8_t>& bytes, bool& found) {
  MetadataScanner scanner;
  for (uint8_t byte : bytes)
    scanner.push(byte);
  found = scanner.found();
  return scanner.metadata();
}

std::vector<uint8_t> imageWith(const Metadata& metadata) {
  std::vector<uint8_t> image(37, 0xA5);
  const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&metadata);
  image.insert(image.end(), bytes, bytes + sizeof(metadata));
  image.insert(image.end(), 19, 0x5A);
  return image;
}

void test_scanner_finds_valid_metadata_inside_image() {
  Metadata expected = makeMetadata("Marauder v8", "esp32c5");
  bool found = false;
  Metadata actual = scan(imageWith(expected), found);
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_STRING(expected.hardware, actual.hardware);
  TEST_ASSERT_EQUAL_STRING(expected.chip, actual.chip);
}

void test_scanner_rejects_invalid_trailer() {
  Metadata metadata = makeMetadata("Marauder v8", "esp32c5");
  metadata.end_magic[0] = 'X';
  bool found = true;
  scan(imageWith(metadata), found);
  TEST_ASSERT_FALSE(found);
}

void test_scanner_recovers_when_false_magic_precedes_metadata() {
  Metadata invalid = makeMetadata("Wrong", "esp32");
  invalid.end_magic[0] = 'X';
  Metadata expected = makeMetadata("Marauder v8", "esp32c5");
  std::vector<uint8_t> image = imageWith(invalid);
  std::vector<uint8_t> valid_image = imageWith(expected);
  image.insert(image.end(), valid_image.begin(), valid_image.end());
  bool found = false;
  Metadata actual = scan(image, found);
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_STRING(expected.hardware, actual.hardware);
}

void test_metadata_match_requires_hardware_and_chip() {
  Metadata current = makeMetadata("Marauder v8", "esp32c5");
  TEST_ASSERT_TRUE(MarauderFirmware::metadataMatches(
    makeMetadata("Marauder v8", "esp32c5"), current));
  TEST_ASSERT_FALSE(MarauderFirmware::metadataMatches(
    makeMetadata("Marauder Mini v3", "esp32c5"), current));
  TEST_ASSERT_FALSE(MarauderFirmware::metadataMatches(
    makeMetadata("Marauder v8", "esp32s3"), current));
}

void test_metadata_requires_terminated_identity_fields() {
  Metadata metadata = makeMetadata("Marauder v8", "esp32c5");
  memset(metadata.hardware, 'A', sizeof(metadata.hardware));
  TEST_ASSERT_FALSE(MarauderFirmware::validMetadata(metadata));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_scanner_finds_valid_metadata_inside_image);
  RUN_TEST(test_scanner_rejects_invalid_trailer);
  RUN_TEST(test_scanner_recovers_when_false_magic_precedes_metadata);
  RUN_TEST(test_metadata_match_requires_hardware_and_chip);
  RUN_TEST(test_metadata_requires_terminated_identity_fields);
  return UNITY_END();
}
