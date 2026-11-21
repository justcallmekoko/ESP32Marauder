#include "IPv4Range.h"

namespace marauder {

namespace {

bool isContiguousMask(uint32_t mask) {
  const uint32_t inverted = ~mask;
  return (inverted & (inverted + 1U)) == 0U;
}

}  // namespace

IPv4HostRange ipv4HostRange(uint32_t address, uint32_t subnetMask) {
  const uint32_t network = address & subnetMask;
  const uint32_t broadcast = network | ~subnetMask;
  const bool valid = isContiguousMask(subnetMask) &&
                     (broadcast - network) >= 2U;

  return {network, broadcast, valid ? network + 1U : 0U,
          valid ? broadcast - 1U : 0U, valid};
}

uint32_t nextIPv4Host(uint32_t current, const IPv4HostRange& range) {
  if (!range.valid || current >= range.last) {
    return 0U;
  }
  if (current < range.first) {
    return range.first;
  }
  return current + 1U;
}

uint32_t previousIPv4Host(uint32_t current, uint32_t steps,
                          const IPv4HostRange& range) {
  if (!range.valid || current < range.first || current > range.last ||
      steps > current - range.first) {
    return 0U;
  }
  return current - steps;
}

}  // namespace marauder
