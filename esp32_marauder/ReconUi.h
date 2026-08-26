#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

constexpr uint32_t RECON_DEVICE_TTL_MS = 2UL * 60UL * 1000UL;
constexpr uint32_t RECON_DEAUTH_ALERT_MS = 3000;
constexpr uint32_t RECON_CHANNEL_PAGE_MS = 4000;
constexpr uint8_t RECON_CHANNELS_PER_PAGE = 8;

enum class ReconLayout : uint8_t {
  COMPACT_SQUARE,
  COMPACT_LANDSCAPE,
  NARROW_PORTRAIT,
  LARGE
};

enum class ReconSignalTrend : int8_t { DEPARTING = -1, STEADY = 0, APPROACHING = 1 };

inline ReconLayout reconLayoutFor(uint16_t width, uint16_t height) {
  if (width <= 135 && height <= 160) return ReconLayout::COMPACT_SQUARE;
  if (height < 160) return ReconLayout::COMPACT_LANDSCAPE;
  if (width < 200) return ReconLayout::NARROW_PORTRAIT;
  return ReconLayout::LARGE;
}

inline uint8_t reconRelationshipRows(ReconLayout layout) {
  return layout == ReconLayout::LARGE ? 3 :
         layout == ReconLayout::NARROW_PORTRAIT ? 1 : 0;
}

inline uint8_t reconChannelPage(uint32_t elapsed_ms, uint8_t channel_count) {
  if (!channel_count) return 0;
  const uint8_t page_count =
      (channel_count + RECON_CHANNELS_PER_PAGE - 1) / RECON_CHANNELS_PER_PAGE;
  return (elapsed_ms / RECON_CHANNEL_PAGE_MS) % page_count;
}

inline uint8_t reconChannelsOnPage(uint8_t page, uint8_t channel_count) {
  const uint16_t first = static_cast<uint16_t>(page) * RECON_CHANNELS_PER_PAGE;
  if (first >= channel_count) return 0;
  const uint8_t remaining = channel_count - first;
  return remaining < RECON_CHANNELS_PER_PAGE ? remaining : RECON_CHANNELS_PER_PAGE;
}

inline uint8_t reconChurnHeight(uint8_t value, uint8_t peak, uint8_t height) {
  if (!value || !height) return 0;
  if (!peak) peak = 1;
  const uint16_t scaled = 1 + (static_cast<uint16_t>(value) * (height - 1)) / peak;
  return scaled > height ? height : scaled;
}

inline uint8_t reconSignalSegments(int8_t rssi) {
  if (rssi <= -127) return 0;
  if (rssi <= -90) return 1;
  if (rssi <= -78) return 2;
  if (rssi <= -67) return 3;
  if (rssi <= -55) return 4;
  return 5;
}

inline uint8_t reconRssiPlotLevel(int8_t rssi) {
  if (rssi <= -127) return 0;
  if (rssi <= -100) return 1;
  if (rssi >= -35) return 100;
  return 1 + ((static_cast<int16_t>(rssi) + 100) * 99) / 65;
}

inline const char* reconProximityLabel(int8_t rssi) {
  if (rssi <= -127) return "NO SIGNAL";
  if (rssi <= -82) return "FAR";
  if (rssi <= -65) return "MID";
  return "NEAR";
}

inline ReconSignalTrend reconSignalTrend(const int8_t* samples, size_t count) {
  if (!samples || count < 2) return ReconSignalTrend::STEADY;
  size_t first = 0;
  while (first < count && samples[first] <= -127) first++;
  if (first == count) return ReconSignalTrend::STEADY;
  size_t last = count - 1;
  while (last > first && samples[last] <= -127) last--;
  if (last == first) return ReconSignalTrend::STEADY;
  const int delta = static_cast<int>(samples[last]) - samples[first];
  if (delta >= 6) return ReconSignalTrend::APPROACHING;
  if (delta <= -6) return ReconSignalTrend::DEPARTING;
  return ReconSignalTrend::STEADY;
}

inline bool reconDeviceExpired(uint32_t now, uint32_t last_seen,
                               uint32_t ttl = RECON_DEVICE_TTL_MS) {
  return last_seen != 0 && static_cast<uint32_t>(now - last_seen) >= ttl;
}

inline void reconTruncate(const char* source, char* output, size_t output_size) {
  if (!output || output_size == 0) return;
  if (!source) source = "";
  const size_t length = strlen(source);
  if (length < output_size) {
    memcpy(output, source, length + 1);
    return;
  }
  if (output_size < 5) {
    memcpy(output, source, output_size - 1);
    output[output_size - 1] = '\0';
    return;
  }
  const size_t visible = output_size - 1;
  const size_t prefix = (visible - 2) / 2;
  const size_t suffix = visible - prefix - 2;
  memcpy(output, source, prefix);
  output[prefix] = '.';
  output[prefix + 1] = '.';
  memcpy(output + prefix + 2, source + length - suffix, suffix);
  output[visible] = '\0';
}
