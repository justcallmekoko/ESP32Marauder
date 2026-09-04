#include "FirmwareMetadata.h"

#ifdef PIO_UNIT_TESTING
  #define HARDWARE_NAME "Native test target"
#else
  #include "configs.h"
#endif

namespace MarauderFirmware {
namespace {

constexpr uint8_t METADATA_MAGIC[METADATA_MAGIC_SIZE] = {
  'M', 'R', 'D', 'R', 'F', 'W', 'I', 'D'
};

constexpr uint8_t METADATA_END_MAGIC[METADATA_MAGIC_SIZE] = {
  'D', 'I', 'W', 'F', 'R', 'D', 'R', 'M'
};

#if defined(CONFIG_IDF_TARGET_ESP32C5)
  #define MARAUDER_CHIP_ID "esp32c5"
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  #define MARAUDER_CHIP_ID "esp32c6"
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  #define MARAUDER_CHIP_ID "esp32s3"
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
  #define MARAUDER_CHIP_ID "esp32s2"
#elif defined(CONFIG_IDF_TARGET_ESP32)
  #define MARAUDER_CHIP_ID "esp32"
#else
  #define MARAUDER_CHIP_ID "native"
#endif

const Metadata CURRENT_METADATA = {
  {'M', 'R', 'D', 'R', 'F', 'W', 'I', 'D'},
  METADATA_SCHEMA_VERSION,
  {0, 0, 0},
  HARDWARE_NAME,
  MARAUDER_CHIP_ID,
  {'D', 'I', 'W', 'F', 'R', 'D', 'R', 'M'}
};

}  // namespace

MetadataScanner::MetadataScanner()
  : candidate{}, magic_index(0), capture_index(0), capturing(false), valid(false) {}

bool MetadataScanner::push(uint8_t byte) {
  if (valid)
    return true;

  if (!capturing) {
    if (byte == METADATA_MAGIC[magic_index]) {
      candidate.magic[magic_index++] = byte;
      if (magic_index == METADATA_MAGIC_SIZE) {
        capturing = true;
        capture_index = METADATA_MAGIC_SIZE;
      }
    }
    else {
      magic_index = byte == METADATA_MAGIC[0] ? 1 : 0;
      if (magic_index == 1)
        candidate.magic[0] = byte;
    }
    return false;
  }

  reinterpret_cast<uint8_t*>(&candidate)[capture_index++] = byte;
  if (capture_index < sizeof(Metadata))
    return false;

  valid = validMetadata(candidate);
  if (!valid) {
    uint8_t replay[sizeof(Metadata) - 1];
    memcpy(replay, reinterpret_cast<const uint8_t*>(&candidate) + 1, sizeof(replay));
    candidate = {};
    magic_index = 0;
    capture_index = 0;
    capturing = false;
    for (size_t i = 0; i < sizeof(replay) && !valid; i++)
      push(replay[i]);
  }
  return valid;
}

bool MetadataScanner::found() const {
  return valid;
}

const Metadata& MetadataScanner::metadata() const {
  return candidate;
}

bool validMetadata(const Metadata& metadata) {
  return memcmp(metadata.magic, METADATA_MAGIC, METADATA_MAGIC_SIZE) == 0 &&
         metadata.schema_version == METADATA_SCHEMA_VERSION &&
         metadata.hardware[0] != '\0' &&
         metadata.chip[0] != '\0' &&
         memchr(metadata.hardware, '\0', sizeof(metadata.hardware)) != nullptr &&
         memchr(metadata.chip, '\0', sizeof(metadata.chip)) != nullptr &&
         memcmp(metadata.end_magic, METADATA_END_MAGIC, METADATA_MAGIC_SIZE) == 0;
}

bool metadataMatches(const Metadata& candidate, const Metadata& current) {
  return validMetadata(candidate) && validMetadata(current) &&
         strncmp(candidate.hardware, current.hardware, sizeof(current.hardware)) == 0 &&
         strncmp(candidate.chip, current.chip, sizeof(current.chip)) == 0;
}

const Metadata& currentMetadata() {
  return CURRENT_METADATA;
}

}  // namespace MarauderFirmware
