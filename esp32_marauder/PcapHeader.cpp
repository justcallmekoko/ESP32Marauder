#include "PcapHeader.h"

namespace marauder {
namespace {

void writeLittleEndian16(uint16_t value, uint8_t* output) {
  output[0] = static_cast<uint8_t>(value);
  output[1] = static_cast<uint8_t>(value >> 8);
}

void writeLittleEndian32(uint32_t value, uint8_t* output) {
  output[0] = static_cast<uint8_t>(value);
  output[1] = static_cast<uint8_t>(value >> 8);
  output[2] = static_cast<uint8_t>(value >> 16);
  output[3] = static_cast<uint8_t>(value >> 24);
}

}  // namespace

void makePcapGlobalHeader(uint32_t snapshot_length,
                          uint8_t output[kPcapGlobalHeaderSize],
                          bool use_radiotap) {
  writeLittleEndian32(0xa1b2c3d4, output);
  writeLittleEndian16(2, output + 4);
  writeLittleEndian16(4, output + 6);
  writeLittleEndian32(0, output + 8);
  writeLittleEndian32(0, output + 12);
  writeLittleEndian32(snapshot_length, output + 16);
  // LinkType 127 = DLT_IEEE802_11_RADIO (Radiotap), 105 = DLT_IEEE802_11
  writeLittleEndian32(use_radiotap ? 127 : 105, output + 20);
}

size_t makeRadiotapHeader(int8_t rssi, uint8_t channel, uint8_t rate,
                          uint8_t output[kRadiotapHeaderSize]) {
  // it_version = 0, it_pad = 0
  output[0] = 0;
  output[1] = 0;
  // it_len = 16 (little-endian)
  writeLittleEndian16(16, output + 2);
  // it_present: bit 1 (Flags: 0x02), bit 2 (Rate: 0x04), bit 3 (Channel: 0x08), bit 5 (dBm Signal: 0x20) = 0x0000002e
  writeLittleEndian32(0x0000002e, output + 4);
  // Flags: 0x00
  output[8] = 0x00;
  // Rate in 500 kbps units (e.g. 2 = 1Mbps)
  output[9] = rate > 0 ? rate : 2;
  // Channel frequency (MHz)
  uint16_t freq = 2412;
  if (channel >= 1 && channel <= 14) {
    if (channel == 14) {
      freq = 2484;
    } else {
      freq = 2407 + (channel * 5);
    }
  } else if (channel > 14) {
    freq = 5000 + (channel * 5);
  }
  writeLittleEndian16(freq, output + 10);
  // Channel flags: 0x00a0 = 2GHz / OFDM, 0x0140 = 5GHz / OFDM
  uint16_t chan_flags = (channel > 14) ? 0x0140 : 0x00a0;
  writeLittleEndian16(chan_flags, output + 12);
  // dBm Antenna Signal (signed RSSI byte)
  output[14] = static_cast<uint8_t>(rssi);
  // Padding byte for 16-byte alignment
  output[15] = 0x00;

  return 16;
}

}  // namespace marauder
