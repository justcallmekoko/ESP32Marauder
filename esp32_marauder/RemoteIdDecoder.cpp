#include "RemoteIdDecoder.h"

#include <cstring>

namespace {
constexpr uint8_t kMessageBasicId = 0;
constexpr uint8_t kMessageLocation = 1;
constexpr uint8_t kMessageAuthentication = 2;
constexpr uint8_t kMessageSelfId = 3;
constexpr uint8_t kMessageSystem = 4;
constexpr uint8_t kMessageOperatorId = 5;
constexpr uint8_t kMessagePack = 15;
constexpr uint8_t kVendorSpecificElement = 221;
constexpr uint8_t kAsdStanOui[] = {0xFA, 0x0B, 0xBC};
constexpr uint8_t kOpenDroneIdVendorType = 0x0D;
constexpr uint8_t kOpenDroneIdBleApplicationCode = 0x0D;
constexpr float kAltitudeOffsetM = 1000.0f;
constexpr float kAltitudeScaleM = 0.5f;

uint32_t readU32Le(const uint8_t* bytes) {
  return static_cast<uint32_t>(bytes[0]) |
         (static_cast<uint32_t>(bytes[1]) << 8) |
         (static_cast<uint32_t>(bytes[2]) << 16) |
         (static_cast<uint32_t>(bytes[3]) << 24);
}
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
      record.heightIsAboveGround = (message[1] & 0x04) != 0;
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
      // ASTM packs horizontal accuracy in the low nibble and vertical
      // accuracy in the high nibble.
      record.horizontalAccuracy = message[19] & 0x0F;
      record.verticalAccuracy = message[19] >> 4;
      record.speedAccuracy = message[20] & 0x0F;
      record.locationTimestampDeciseconds = readU16Le(message + 21);
      record.directionValid = record.directionDeg <= 360;
      record.horizontalSpeedValid = record.horizontalSpeedMps <= 254.25f;
      record.verticalSpeedValid = record.verticalSpeedMps >= -62.0f &&
                                  record.verticalSpeedMps <= 62.0f;
      record.altitudePressureValid = record.altitudePressureM > -1000.0f;
      record.altitudeGeoValid = record.altitudeGeoM > -1000.0f;
      record.heightValid = record.heightM > -1000.0f;
      record.locationTimestampValid = record.locationTimestampDeciseconds != 0xFFFF;
      record.hasLocation = remoteIdCoordinatesValid(record.latitudeE7,
                                                     record.longitudeE7);
      return RemoteIdDecodeResult::Decoded;
    }

    case kMessageAuthentication:
      record.authenticationType = message[1] >> 4;
      record.authenticationPage = message[1] & 0x0F;
      if (record.authenticationPage == 0) {
        record.authenticationLastPage = message[2];
        record.authenticationLength = message[3];
        record.authenticationTimestamp = readU32Le(message + 4);
        if (record.authenticationLastPage > 15)
          return RemoteIdDecodeResult::Malformed;
      }
      record.hasAuthentication = true;
      return RemoteIdDecodeResult::Decoded;

    case kMessageSelfId:
      copyText(record.description, sizeof(record.description), message + 2, 23);
      record.hasDescription = record.description[0] != '\0';
      return RemoteIdDecodeResult::Decoded;

    case kMessageSystem:
      record.operatorLocationType = message[1] & 0x03;
      record.classificationType = (message[1] >> 2) & 0x07;
      record.operatorLatitudeE7 = readI32Le(message + 2);
      record.operatorLongitudeE7 = readI32Le(message + 6);
      record.areaCount = readU16Le(message + 10);
      record.areaRadiusM = static_cast<uint16_t>(message[12]) * 10U;
      record.areaCeilingM = readU16Le(message + 13) * kAltitudeScaleM -
                            kAltitudeOffsetM;
      record.areaFloorM = readU16Le(message + 15) * kAltitudeScaleM -
                          kAltitudeOffsetM;
      record.classEu = message[17] & 0x0F;
      record.categoryEu = message[17] >> 4;
      record.operatorAltitudeGeoM = readU16Le(message + 18) *
                                    kAltitudeScaleM - kAltitudeOffsetM;
      record.systemTimestamp = readU32Le(message + 20);
      record.areaCeilingValid = record.areaCeilingM > -1000.0f;
      record.areaFloorValid = record.areaFloorM > -1000.0f;
      record.operatorAltitudeValid = record.operatorAltitudeGeoM > -1000.0f;
      record.hasOperatorLocation = remoteIdCoordinatesValid(
          record.operatorLatitudeE7, record.operatorLongitudeE7);
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
  // ASTM Bluetooth 4 Service Data is UUID, application code (0x0D), message
  // counter, then the 25-byte Open Drone ID message. Skipping only the counter
  // shifts every decoded field by one byte and can still yield plausible but
  // entirely false telemetry.
  if (length < 4 + kMessageSize) return RemoteIdDecodeResult::Malformed;
  if (serviceData[2] != kOpenDroneIdBleApplicationCode)
    return RemoteIdDecodeResult::Ignored;
  return decodePayload(serviceData + 4, length - 4, record);
}

RemoteIdDecodeResult RemoteIdDecoder::decodeWifiNan(
    const uint8_t* frame, size_t length, RemoteIdRecord& record) const {
  static constexpr uint8_t kNanDestination[] = {0x51, 0x6F, 0x9A, 0x01, 0x00, 0x00};
  static constexpr uint8_t kWifiAllianceOui[] = {0x50, 0x6F, 0x9A};
  static constexpr uint8_t kRemoteIdService[] = {0x88, 0x69, 0x19, 0x9D, 0x92, 0x09};
  if (frame == nullptr || length < 44) return RemoteIdDecodeResult::Malformed;
  if ((frame[0] & 0xFC) != 0xD0) return RemoteIdDecodeResult::Ignored;
  if (std::memcmp(frame + 4, kNanDestination, sizeof(kNanDestination)) != 0)
    return RemoteIdDecodeResult::Ignored;
  const size_t nan = 24;
  if (frame[nan] != 0x04 || frame[nan + 1] != 0x09 ||
      std::memcmp(frame + nan + 2, kWifiAllianceOui, sizeof(kWifiAllianceOui)) != 0 ||
      frame[nan + 5] != 0x13)
    return RemoteIdDecodeResult::Ignored;
  const size_t descriptor = nan + 6;
  if (frame[descriptor] != 0x03 ||
      std::memcmp(frame + descriptor + 3, kRemoteIdService,
                  sizeof(kRemoteIdService)) != 0 ||
      frame[descriptor + 9] != 0x01 || frame[descriptor + 11] != 0x10)
    return RemoteIdDecodeResult::Ignored;
  const size_t serviceLength = frame[descriptor + 12];
  const size_t serviceInfo = descriptor + 13;
  if (serviceLength < 1 || serviceInfo + serviceLength > length)
    return RemoteIdDecodeResult::Malformed;
  return decodePayload(frame + serviceInfo + 1, serviceLength - 1, record);
}
