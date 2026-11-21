#include "BeaconFrame.h"

#include <string.h>

namespace {

constexpr uint8_t kBroadcastAddress[6] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

constexpr uint8_t kRates24Ghz[] = {
  0x01, 0x08,
  0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24
};

constexpr uint8_t kExtendedRates24Ghz[] = {
  0x32, 0x04, 0x30, 0x48, 0x60, 0x6c
};

constexpr uint8_t kRates5Ghz[] = {
  0x01, 0x08,
  0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c
};

void writeLittleEndian16(uint8_t* target, uint16_t value) {
  target[0] = value & 0xff;
  target[1] = value >> 8;
}

void writeLittleEndian64(uint8_t* target, uint64_t value) {
  for (size_t i = 0; i < 8; ++i)
    target[i] = (value >> (i * 8)) & 0xff;
}

bool appendBytes(
  uint8_t* frame,
  size_t capacity,
  size_t& offset,
  const uint8_t* bytes,
  size_t length
) {
  if ((frame == nullptr) || (bytes == nullptr) || (offset + length > capacity))
    return false;
  memcpy(frame + offset, bytes, length);
  offset += length;
  return true;
}

}  // namespace

namespace marauder {

bool isFiveGhzBeaconChannel(uint8_t channel) {
  return channel > 14;
}

void makeLocalUnicastBssid(uint8_t bssid[6]) {
  if (bssid == nullptr)
    return;
  bssid[0] = (bssid[0] & 0xfc) | 0x02;
}

bool beaconCycleExpired(
  uint32_t current_time,
  uint32_t cycle_started,
  uint32_t cycle_duration
) {
  return static_cast<uint32_t>(current_time - cycle_started) >= cycle_duration;
}

size_t buildOpenBeaconFrame(
  uint8_t* frame,
  size_t frame_capacity,
  const BeaconIdentity& identity,
  uint64_t timestamp_us
) {
  if ((frame == nullptr) ||
      (identity.ssid_length > kMaxBeaconSsidLength) ||
      (identity.channel == 0))
    return 0;

  constexpr size_t kFixedHeaderLength = 36;
  if (frame_capacity < kFixedHeaderLength)
    return 0;

  memset(frame, 0, frame_capacity);
  frame[0] = 0x80;
  memcpy(frame + 4, kBroadcastAddress, sizeof(kBroadcastAddress));
  memcpy(frame + 10, identity.bssid, sizeof(identity.bssid));
  memcpy(frame + 16, identity.bssid, sizeof(identity.bssid));
  writeLittleEndian16(frame + 22, (identity.sequence & 0x0fff) << 4);
  writeLittleEndian64(frame + 24, timestamp_us);
  writeLittleEndian16(frame + 32, 100);
  writeLittleEndian16(frame + 34, 0x0001);  // ESS, open network.

  size_t offset = kFixedHeaderLength;
  const uint8_t ssid_header[] = {0x00, identity.ssid_length};
  if (!appendBytes(frame, frame_capacity, offset, ssid_header, sizeof(ssid_header)) ||
      !appendBytes(
        frame,
        frame_capacity,
        offset,
        reinterpret_cast<const uint8_t*>(identity.ssid),
        identity.ssid_length
      ))
    return 0;

  if (isFiveGhzBeaconChannel(identity.channel)) {
    if (!appendBytes(frame, frame_capacity, offset, kRates5Ghz, sizeof(kRates5Ghz)))
      return 0;
  } else {
    const uint8_t ds_parameter[] = {0x03, 0x01, identity.channel};
    if (!appendBytes(frame, frame_capacity, offset, kRates24Ghz, sizeof(kRates24Ghz)) ||
        !appendBytes(frame, frame_capacity, offset, ds_parameter, sizeof(ds_parameter)) ||
        !appendBytes(
          frame,
          frame_capacity,
          offset,
          kExtendedRates24Ghz,
          sizeof(kExtendedRates24Ghz)
        ))
      return 0;
  }

  return offset;
}

}  // namespace marauder

bool setBeaconFrameChannel(
  uint8_t* frame,
  size_t frame_size,
  size_t ssid_length,
  uint8_t channel
) {
  const size_t channel_index = 50 + ssid_length;
  if ((frame == nullptr) || (channel_index >= frame_size))
    return false;

  frame[channel_index] = channel;
  return true;
}
