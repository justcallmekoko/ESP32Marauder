#include "FlockDetector.h"

#include <string.h>

namespace {
constexpr size_t kProbeHeaderLength = 24;
constexpr uint8_t kSsidTag = 0;
constexpr uint8_t kVendorTag = 221;
constexpr uint8_t kHtCapabilitiesTag = 45;
constexpr uint8_t kVhtCapabilitiesTag = 191;
constexpr uint8_t kLiteOnFingerprint[] = {0x50, 0x6f, 0x9a, 0x16, 0x03, 0x01, 0x03};
constexpr uint8_t kWpaFingerprint[] = {0x00, 0x50, 0xf2, 0x08, 0x00, 0x00, 0x00};

bool tlvFits(size_t offset, size_t value_len, size_t body_len) {
  return offset <= body_len && value_len <= body_len - offset && 2 <= body_len - offset - value_len;
}

bool vendorMatches(const uint8_t* body, size_t body_len, size_t offset,
                   const uint8_t* fingerprint, size_t fingerprint_len) {
  return offset + 2 <= body_len && body[offset] == kVendorTag &&
         body[offset + 1] == fingerprint_len &&
         tlvFits(offset, fingerprint_len, body_len) &&
         memcmp(body + offset + 2, fingerprint, fingerprint_len) == 0;
}

bool exactSuffixMatches(const uint8_t* body, size_t body_len, size_t offset) {
  if (!vendorMatches(body, body_len, offset, kLiteOnFingerprint, sizeof(kLiteOnFingerprint)))
    return false;
  offset += 2 + sizeof(kLiteOnFingerprint);

  const uint8_t tags[] = {kHtCapabilitiesTag, kVhtCapabilitiesTag};
  for (uint8_t tag : tags) {
    if (offset + 2 > body_len || body[offset] != tag)
      return false;
    size_t value_len = body[offset + 1];
    if (!tlvFits(offset, value_len, body_len))
      return false;
    offset += 2 + value_len;
  }

  if (!vendorMatches(body, body_len, offset, kWpaFingerprint, sizeof(kWpaFingerprint)))
    return false;
  return offset + 2 + sizeof(kWpaFingerprint) == body_len;
}

bool primaryBodyMatches(const uint8_t* body, size_t body_len) {
  if (body_len < 2 || body[0] != kSsidTag || body[1] != 0)
    return false;

  size_t offset = 2;
  while (offset + 2 <= body_len && body[offset] == kSsidTag && body[offset + 1] == 0)
    offset += 2;

  while (offset + 2 <= body_len) {
    size_t value_len = body[offset + 1];
    if (!tlvFits(offset, value_len, body_len))
      return false;
    if (exactSuffixMatches(body, body_len, offset))
      return true;
    offset += 2 + value_len;
  }
  return false;
}
}  // namespace

bool flockProbeBodyMatches(const uint8_t* body, size_t body_len) {
  if (body == nullptr)
    return false;
  if (primaryBodyMatches(body, body_len))
    return true;
  return body_len > 4 && primaryBodyMatches(body, body_len - 4);
}

bool flockProbeRequestMatches(const uint8_t* frame, size_t frame_len) {
  if (frame == nullptr || frame_len < kProbeHeaderLength + 2)
    return false;
  uint16_t frame_control = (uint16_t)frame[0] | ((uint16_t)frame[1] << 8);
  if ((frame_control & 0x000c) != 0 || (frame_control & 0x00f0) != 0x0040)
    return false;
  return flockProbeBodyMatches(frame + kProbeHeaderLength, frame_len - kProbeHeaderLength);
}

