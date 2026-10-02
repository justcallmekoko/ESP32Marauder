#pragma once

#include <stddef.h>
#include <stdint.h>

enum class RemoteIdTransport : uint8_t {
  None = 0,
  WifiBeacon = 1 << 0,
  WifiNan = 1 << 1,
  BleLegacy = 1 << 2,
  BleExtended = 1 << 3,
};

struct RemoteIdRecord {
  char uasId[21] = {};
  char operatorId[21] = {};
  char description[24] = {};
  uint8_t mac[6] = {};
  uint8_t transportMask = 0;
  uint8_t idType = 0;
  uint8_t uaType = 0;
  uint8_t operationalStatus = 0;
  uint8_t operatorLocationType = 0;
  uint8_t classificationType = 0;
  uint8_t categoryEu = 0;
  uint8_t classEu = 0;
  uint8_t horizontalAccuracy = 0;
  uint8_t verticalAccuracy = 0;
  uint8_t speedAccuracy = 0;
  uint8_t lastWifiChannel = 0;
  bool hasUasId = false;
  bool hasOperatorId = false;
  bool hasDescription = false;
  bool hasLocation = false;
  bool hasOperatorLocation = false;
  bool hasAuthentication = false;
  bool directionValid = false;
  bool horizontalSpeedValid = false;
  bool verticalSpeedValid = false;
  bool altitudePressureValid = false;
  bool altitudeGeoValid = false;
  bool heightValid = false;
  bool locationTimestampValid = false;
  bool areaCeilingValid = false;
  bool areaFloorValid = false;
  bool operatorAltitudeValid = false;
  uint8_t authenticationType = 0;
  uint8_t authenticationPage = 0;
  uint8_t authenticationLastPage = 0;
  uint8_t authenticationLength = 0;
  uint32_t authenticationTimestamp = 0;
  uint16_t areaCount = 0;
  uint16_t areaRadiusM = 0;
  float areaCeilingM = 0.0f;
  float areaFloorM = 0.0f;
  float operatorAltitudeGeoM = 0.0f;
  uint32_t systemTimestamp = 0;
  int32_t latitudeE7 = 0;
  int32_t longitudeE7 = 0;
  int32_t operatorLatitudeE7 = 0;
  int32_t operatorLongitudeE7 = 0;
  float altitudeGeoM = 0.0f;
  float altitudePressureM = 0.0f;
  float heightM = 0.0f;
  float horizontalSpeedMps = 0.0f;
  float verticalSpeedMps = 0.0f;
  uint16_t directionDeg = 0;
  uint16_t locationTimestampDeciseconds = 0;
  bool heightIsAboveGround = false;
  int8_t rssi = 0;
  uint32_t firstSeenMs = 0;
  uint32_t lastSeenMs = 0;
  uint32_t packetCount = 0;
  uint32_t lastLoggedMs = 0;
  uint32_t rateWindowStartedMs = 0;
  uint16_t rateWindowPackets = 0;
  float packetRateHz = 0.0f;
  bool isLost = false;
  uint16_t lostCount = 0;
  uint16_t reacquiredCount = 0;
  uint32_t lastLostMs = 0;
  uint32_t lastReacquiredMs = 0;
};

struct RemoteIdGridPoint {
  int16_t x = 0;
  int16_t y = 0;
  bool visible = false;
  bool offGrid = false;
};

enum class RemoteIdLayout : uint8_t {
  Compact,
  ListOnly,
  SplitListGrid,
};

class RemoteIdStore {
 public:
  RemoteIdStore(RemoteIdRecord* records, size_t capacity);

  RemoteIdRecord& observe(const uint8_t mac[6], RemoteIdTransport transport,
                          int8_t rssi, uint32_t nowMs);
  RemoteIdRecord* findByUasId(const char* uasId);
  const RemoteIdRecord* findByUasId(const char* uasId) const;
  RemoteIdRecord* findByMac(const uint8_t mac[6]);
  const RemoteIdRecord* findByMac(const uint8_t mac[6]) const;
  RemoteIdRecord* at(size_t index);
  const RemoteIdRecord* at(size_t index) const;
  size_t size() const;
  size_t capacity() const;
  size_t pruneStale(uint32_t nowMs, uint32_t staleAfterMs,
                    const char* preservedUasId = nullptr,
                    const uint8_t* preservedMac = nullptr);
  size_t updateLifecycle(uint32_t nowMs, uint32_t lostAfterMs);
  bool eraseByMac(const uint8_t mac[6], const RemoteIdRecord* except = nullptr);
  void clear();

 private:
  RemoteIdRecord* records_;
  size_t capacity_;
  size_t size_ = 0;
};

RemoteIdLayout remoteIdLayoutForDisplay(uint16_t width, uint16_t height);
float remoteIdDistanceMeters(int32_t latAE7, int32_t lonAE7,
                             int32_t latBE7, int32_t lonBE7);
float remoteIdBearingDegrees(int32_t latAE7, int32_t lonAE7,
                             int32_t latBE7, int32_t lonBE7);
bool remoteIdIsStale(uint32_t nowMs, uint32_t lastSeenMs,
                     uint32_t staleAfterMs);
bool remoteIdCoordinatesValid(int32_t latitudeE7, int32_t longitudeE7);
bool remoteIdShouldReplaceBasicId(uint8_t currentType, bool hasCurrent,
                                  uint8_t candidateType);
void remoteIdFormatDistanceKm(float distanceM, char* output,
                              size_t outputSize);
void remoteIdFormatGridRadius(float radiusM, char* output,
                              size_t outputSize);
void remoteIdFormatTransports(uint8_t transportMask, char* output,
                              size_t outputSize);
float remoteIdGridScaleMeters(const RemoteIdRecord* records, size_t count,
                              int32_t originLatE7, int32_t originLonE7,
                              float minimumRadiusM = 50.0f);
RemoteIdGridPoint remoteIdProjectToGrid(int32_t pointLatE7, int32_t pointLonE7,
                                        int32_t originLatE7, int32_t originLonE7,
                                        int16_t width, int16_t height,
                                        float radiusM, int16_t padding = 6);
