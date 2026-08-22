#pragma once

#include <stddef.h>
#include <stdint.h>

namespace marauder {

constexpr size_t kPcapGlobalHeaderSize = 24;
constexpr size_t kRadiotapHeaderSize = 16;

void makePcapGlobalHeader(uint32_t snapshot_length,
                          uint8_t output[kPcapGlobalHeaderSize],
                          bool use_radiotap = true);

size_t makeRadiotapHeader(int8_t rssi, uint8_t channel, uint8_t rate,
                          uint8_t output[kRadiotapHeaderSize]);

}  // namespace marauder
