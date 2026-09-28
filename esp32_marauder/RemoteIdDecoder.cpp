#include "RemoteIdDecoder.h"

#include <cstring>

namespace {
constexpr uint8_t kMessageBasicId = 0;
constexpr uint8_t kMessageLocation = 1;
constexpr uint8_t kMessageSelfId = 3;
constexpr uint8_t kMessageSystem = 4;
constexpr uint8_t kMessageOperatorId = 5;
constexpr uint8_t kMessagePack = 15;
constexpr uint8_t kVendorSpecificElement = 221;
constexpr uint8_t kAsdStanOui[] = {0xFA, 0x0B, 0xBC};
constexpr uint8_t kOpenDroneIdVendorType = 0x0D;
constexpr float kAltitudeOffsetM = 1000.0f;
constexpr float kAltitudeScaleM = 0.5f;
}

int32_t RemoteIdDecoder::readI32Le(const uint8_t* bytes) {
  const uint32_t value = static_cast<uint32_t>(bytes[0]) |
                         (static_cast<uint32_t>(bytes[1]) << 8) |
                         (static_cast<uint32_t>(bytes[2]) << 16) |
                         (static_cast<uint32_t>(bytes[3]) << 24);
  return static_cast<int32_t>(value);
}

uint16_t RemoteIdDecoder::readU16Le(const uint8_t* bytes) {
  return static_cast<uint16_t>(bytes[0]) |
         static_cast<uint16_t>(bytes[1] << 8);
}

void RemoteIdDecoder::copyText(char* destination, size_t destinationSize,
                               const uint8_t* source, size_t sourceSize) {
  if (destinationSize == 0) return;
  const size_t copyLength = sourceSize < destinationSize - 1
                                ? sourceSize
                                : destinationSize - 1;
  size_t length = 0;
  while (length < copyLength && source[length] != 0) ++length;
  std::memcpy(destination, source, length);
  destination[length] = '\0';
}

RemoteIdDecodeResult RemoteIdDecoder::decodeMessage(
    const uint8_t* message, size_t length, RemoteIdRecord& record) const {
  if (message == nullptr || length < kMessageSize) return RemoteIdDecodeResult::Malformed;
  const uint8_t type = message[0] >> 4;

  switch (type) {
    case kMessageBasicId:
      record.idType = message[1] >> 4;
      record.uaType = message[1] & 0x0F;
      copyText(record.uasId, sizeof(record.uasId), message + 2, 20);
      record.hasUasId = record.uasId[0] != '\0';
      return RemoteIdDecodeResult::Decoded;

    case kMessageLocation: {
      record.operationalStatus = message[1] >> 4;
      const bool highSpeed = (message[1] & 0x01) != 0;
      const bool westDirection = (message[1] & 0x02) != 0;
      record.directionDeg = static_cast<uint16_t>(message[2]) +
                            (westDirection ? 180U : 0U);
      record.horizontalSpeedMps = highSpeed
          ? static_cast<float>(message[3]) * 0.75f + 63.75f
          : static_cast<float>(message[3]) * 0.25f;
      record.verticalSpeedMps = static_cast<int8_t>(message[4]) * 0.5f;
      record.latitudeE7 = readI32Le(message + 5);
      record.longitudeE7 = readI32Le(message + 9);
      record.altitudePressureM = readU16Le(message + 13) * kAltitudeScaleM -
                                 kAltitudeOffsetM;
      record.altitudeGeoM = readU16Le(message + 15) * kAltitudeScaleM -
                            kAltitudeOffsetM;
      record.heightM = readU16Le(message + 17) * kAltitudeScaleM -
                       kAltitudeOffsetM;
      record.hasLocation = record.latitudeE7 != 0 || record.longitudeE7 != 0;
      return RemoteIdDecodeResult::Decoded;
    }

    case kMessageSelfId:
      copyText(record.description, sizeof(record.description), message + 2, 23);
      record.hasDescription = record.description[0] != '\0';
      return RemoteIdDecodeResult::Decoded;

    case kMessageSystem:
      record.operatorLocationType = message[1] & 0x03;
      record.operatorLatitudeE7 = readI32Le(message + 2);
      record.operatorLongitudeE7 = readI32Le(message + 6);
      record.hasOperatorLocation = record.operatorLatitudeE7 != 0 ||
                                   record.operatorLongitudeE7 != 0;
      return RemoteIdDecodeResult::Decoded;

    case kMessageOperatorId:
      copyText(record.operatorId, sizeof(record.operatorId), message + 2, 20);
      record.hasOperatorId = record.operatorId[0] != '\0';
      return RemoteIdDecodeResult::Decoded;

    default:
      return RemoteIdDecodeResult::Ignored;
  }
}

RemoteIdDecodeResult RemoteIdDecoder::decodePayload(
    const uint8_t* payload, size_t length, RemoteIdRecord& record) const {
  if (payload == nullptr || length == 0) return RemoteIdDecodeResult::Malformed;
  if ((payload[0] >> 4) != kMessagePack)
    return decodeMessage(payload, length, record);

  if (length < 3 || payload[1] != kMessageSize || payload[2] == 0)
    return RemoteIdDecodeResult::Malformed;
  const size_t required = 3U + static_cast<size_t>(payload[2]) * kMessageSize;
  if (required > length) return RemoteIdDecodeResult::Malformed;

  bool decoded = false;
  for (uint8_t i = 0; i < payload[2]; ++i) {
    const RemoteIdDecodeResult result =
        decodeMessage(payload + 3U + static_cast<size_t>(i) * kMessageSize,
                      kMessageSize, record);
    if (result == RemoteIdDecodeResult::Malformed) return result;
    decoded |= result == RemoteIdDecodeResult::Decoded;
  }
  return decoded ? RemoteIdDecodeResult::Decoded : RemoteIdDecodeResult::Ignored;
}

RemoteIdDecodeResult RemoteIdDecoder::decodeWifiBeacon(
    const uint8_t* frame, size_t length, RemoteIdRecord& record) const {
  // 24-byte management header + 12-byte fixed beacon parameters.
  if (frame == nullptr || length < 36) return RemoteIdDecodeResult::Malformed;
  if ((frame[0] & 0xFC) != 0x80) return RemoteIdDecodeResult::Ignored;

  size_t offset = 36;
  while (offset + 2 <= length) {
    const uint8_t id = frame[offset];
    const size_t elementLength = frame[offset + 1];
    offset += 2;
    if (offset + elementLength > length) return RemoteIdDecodeResult::Malformed;
    if (id == kVendorSpecificElement && elementLength >= 8 &&
        std::memcmp(frame + offset, kAsdStanOui, sizeof(kAsdStanOui)) == 0 &&
        frame[offset + 3] == kOpenDroneIdVendorType) {
      // OUI, vendor type, and message counter precede the ODID payload.
      return decodePayload(frame + offset + 5, elementLength - 5, record);
    }
    offset += elementLength;
  }
  return RemoteIdDecodeResult::Ignored;
}

RemoteIdDecodeResult RemoteIdDecoder::decodeBleServiceData(
    const uint8_t* serviceData, size_t length, RemoteIdRecord& record) const {
  if (serviceData == nullptr || length < 2) return RemoteIdDecodeResult::Malformed;
  if (readU16Le(serviceData) != kBleServiceUuid) return RemoteIdDecodeResult::Ignored;
  // ASTM Bluetooth 4 Service Data has UUID + one-byte message counter.
  if (length < 3 + kMessageSize) return RemoteIdDecodeResult::Malformed;
  return decodePayload(serviceData + 3, length - 3, record);
}
