#pragma once

#include <stddef.h>
#include <stdint.h>

#include "RemoteIdModel.h"

enum class RemoteIdDecodeResult : uint8_t {
  Decoded,
  Ignored,
  Malformed,
};

class RemoteIdDecoder {
 public:
  static constexpr size_t kMessageSize = 25;
  static constexpr uint16_t kBleServiceUuid = 0xFFFA;

  RemoteIdDecodeResult decodeMessage(const uint8_t* message, size_t length,
                                     RemoteIdRecord& record) const;
  RemoteIdDecodeResult decodePayload(const uint8_t* payload, size_t length,
                                     RemoteIdRecord& record) const;

  // Accepts a complete 802.11 management frame (without the radio metadata).
  RemoteIdDecodeResult decodeWifiBeacon(const uint8_t* frame, size_t length,
                                        RemoteIdRecord& record) const;
  RemoteIdDecodeResult decodeWifiNan(const uint8_t* frame, size_t length,
                                     RemoteIdRecord& record) const;

  // Accepts the value of a BLE Service Data AD structure, including UUID.
  RemoteIdDecodeResult decodeBleServiceData(const uint8_t* serviceData,
                                            size_t length,
                                            RemoteIdRecord& record) const;

 private:
  static int32_t readI32Le(const uint8_t* bytes);
  static uint16_t readU16Le(const uint8_t* bytes);
  static void copyText(char* destination, size_t destinationSize,
                       const uint8_t* source, size_t sourceSize);
};
