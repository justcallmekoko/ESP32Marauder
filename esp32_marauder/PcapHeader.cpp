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
                          uint8_t output[kPcapGlobalHeaderSize]) {
  writeLittleEndian32(0xa1b2c3d4, output);
  writeLittleEndian16(2, output + 4);
  writeLittleEndian16(4, output + 6);
  writeLittleEndian32(0, output + 8);
  writeLittleEndian32(0, output + 12);
  writeLittleEndian32(snapshot_length, output + 16);
  writeLittleEndian32(105, output + 20);
}

}  // namespace marauder
