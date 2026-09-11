#pragma once

#include <cstdint>

namespace marauder {

struct IPv4HostRange {
  uint32_t network;
  uint32_t broadcast;
  uint32_t first;
  uint32_t last;
  bool valid;
};

IPv4HostRange ipv4HostRange(uint32_t address, uint32_t subnetMask);
uint32_t nextIPv4Host(uint32_t current, const IPv4HostRange& range);
uint32_t previousIPv4Host(uint32_t current, uint32_t steps,
                          const IPv4HostRange& range);

}  // namespace marauder
