#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace MarauderFirmware {

constexpr size_t METADATA_MAGIC_SIZE = 8;
constexpr size_t METADATA_HARDWARE_SIZE = 48;
constexpr size_t METADATA_CHIP_SIZE = 12;
constexpr uint8_t METADATA_SCHEMA_VERSION = 1;

struct __attribute__((packed)) Metadata {
  uint8_t magic[METADATA_MAGIC_SIZE];
  uint8_t schema_version;
  uint8_t reserved[3];
  char hardware[METADATA_HARDWARE_SIZE];
  char chip[METADATA_CHIP_SIZE];
  uint8_t end_magic[METADATA_MAGIC_SIZE];
};

class MetadataScanner {
 public:
  MetadataScanner();

  bool push(uint8_t byte);
  bool found() const;
  const Metadata& metadata() const;

 private:
  Metadata candidate;
  size_t magic_index;
  size_t capture_index;
  bool capturing;
  bool valid;
};

bool validMetadata(const Metadata& metadata);
bool metadataMatches(const Metadata& candidate, const Metadata& current);
const Metadata& currentMetadata();

}  // namespace MarauderFirmware
