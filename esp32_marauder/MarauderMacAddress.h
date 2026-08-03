#pragma once

#include <stddef.h>
#include <stdint.h>

namespace marauder {

constexpr size_t kMacAddressSize = 6;
constexpr size_t kMacAddressTextLength = 17;

bool formatMacAddress(const uint8_t mac[kMacAddressSize],
                      char output[kMacAddressTextLength + 1]);

bool parseMacAddress(const char* text, uint8_t output[kMacAddressSize]);

}  // namespace marauder
