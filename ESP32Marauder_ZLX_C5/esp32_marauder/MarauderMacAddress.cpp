#include "MarauderMacAddress.h"

#include <string.h>

namespace marauder {
namespace {

constexpr char kHexDigits[] = "0123456789ABCDEF";

int hexValue(char value) {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }
  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }
  return -1;
}

}  // namespace

bool formatMacAddress(const uint8_t mac[kMacAddressSize],
                      char output[kMacAddressTextLength + 1]) {
  if (mac == nullptr || output == nullptr) {
    return false;
  }

  for (size_t index = 0; index < kMacAddressSize; ++index) {
    const size_t offset = index * 3;
    output[offset] = kHexDigits[(mac[index] >> 4) & 0x0F];
    output[offset + 1] = kHexDigits[mac[index] & 0x0F];
    if (index + 1 < kMacAddressSize) {
      output[offset + 2] = ':';
    }
  }
  output[kMacAddressTextLength] = '\0';
  return true;
}

bool parseMacAddress(const char* text, uint8_t output[kMacAddressSize]) {
  if (text == nullptr || output == nullptr ||
      strlen(text) != kMacAddressTextLength) {
    return false;
  }

  uint8_t parsed[kMacAddressSize] = {};
  for (size_t index = 0; index < kMacAddressSize; ++index) {
    const size_t offset = index * 3;
    const int high = hexValue(text[offset]);
    const int low = hexValue(text[offset + 1]);
    if (high < 0 || low < 0) {
      return false;
    }
    if (index + 1 < kMacAddressSize && text[offset + 2] != ':') {
      return false;
    }
    parsed[index] = static_cast<uint8_t>((high << 4) | low);
  }

  memcpy(output, parsed, sizeof(parsed));
  return true;
}

}  // namespace marauder
