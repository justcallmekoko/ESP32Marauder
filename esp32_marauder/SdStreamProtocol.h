#pragma once

#include <stddef.h>
#include <stdint.h>

namespace marauder {

constexpr size_t kSdStreamChunkSize = 192;
constexpr size_t kSdStreamEncodedSize = ((kSdStreamChunkSize + 2) / 3) * 4;
constexpr size_t kSdStreamEncodedBufferSize = kSdStreamEncodedSize + 1;
constexpr uint32_t kSdStreamCrc32Initial = 0xffffffffU;

inline uint32_t sdStreamCrc32Update(uint32_t crc, const uint8_t* data, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
  }
  return crc;
}

inline uint32_t sdStreamCrc32Finish(uint32_t crc) {
  return crc ^ 0xffffffffU;
}

}  // namespace marauder
