#include "RemoteIdModel.h"

#include <cmath>
#include <climits>
#include <cstdio>
#include <cstring>

namespace {
constexpr double kEarthRadiusM = 6371000.0;
constexpr double kDegToRad = 0.017453292519943295;

bool sameMac(const uint8_t a[6], const uint8_t b[6]) {
  return std::memcmp(a, b, 6) == 0;
}

uint8_t transportBit(RemoteIdTransport transport) {
  return static_cast<uint8_t>(transport);
}

void relativeMeters(int32_t latE7, int32_t lonE7,
                    int32_t originLatE7, int32_t originLonE7,
                    double& eastM, double& northM) {
  const double originLatRad = static_cast<double>(originLatE7) * 1e-7 * kDegToRad;
  northM = static_cast<double>(latE7 - originLatE7) * 1e-7 * kDegToRad * kEarthRadiusM;
  eastM = static_cast<double>(lonE7 - originLonE7) * 1e-7 * kDegToRad *
          kEarthRadiusM * std::cos(originLatRad);
}

int16_t clampMeters(double value) {
  if (value > INT16_MAX) return INT16_MAX;
  if (value < INT16_MIN) return INT16_MIN;
  return static_cast<int16_t>(std::lround(value));
}

RemoteIdGridPoint projectMetersToGrid(double eastM, double northM,
                                      int16_t width, int16_t height,
                                      float radiusM, int16_t padding) {
  RemoteIdGridPoint point;
  if (width <= padding * 2 || height <= padding * 2 || radiusM <= 0.0f)
    return point;
  const double halfW = (width - padding * 2) * 0.5;
  const double halfH = (height - padding * 2) * 0.5;
  point.x = static_cast<int16_t>(std::lround(width * 0.5 + eastM / radiusM * halfW));
  point.y = static_cast<int16_t>(std::lround(height * 0.5 - northM / radiusM * halfH));
  point.visible = point.x >= padding && point.x < width - padding &&
                  point.y >= padding && point.y < height - padding;
  point.offGrid = !point.visible;
  if (point.offGrid) {
    if (point.x < padding) point.x = padding;
    if (point.x >= width - padding) point.x = width - padding - 1;
    if (point.y < padding) point.y = padding;
    if (point.y >= height - padding) point.y = height - padding - 1;
  }
  return point;
}
}  // namespace

RemoteIdStore::RemoteIdStore(RemoteIdRecord* records, size_t capacity)
    : records_(records), capacity_(capacity) {}

RemoteIdRecord& RemoteIdStore::observe(const uint8_t mac[6],
                                        RemoteIdTransport transport,
                                        int8_t rssi, uint32_t nowMs) {
  for (size_t i = 0; i < size_; ++i) {
    if (sameMac(records_[i].mac, mac)) {
      if (records_[i].isLost) {
        records_[i].isLost = false;
        ++records_[i].reacquiredCount;
        records_[i].lastReacquiredMs = nowMs;
      }
      records_[i].transportMask |= transportBit(transport);
      records_[i].rssi = rssi;
      records_[i].lastSeenMs = nowMs;
      ++records_[i].packetCount;
      if (records_[i].rateWindowStartedMs == 0)
        records_[i].rateWindowStartedMs = nowMs;
      ++records_[i].rateWindowPackets;
      const uint32_t elapsed = nowMs - records_[i].rateWindowStartedMs;
      if (elapsed >= 1000) {
        records_[i].packetRateHz = records_[i].rateWindowPackets * 1000.0f /
                                   static_cast<float>(elapsed);
        records_[i].rateWindowStartedMs = nowMs;
        records_[i].rateWindowPackets = 0;
      }
      return records_[i];
    }
  }

  size_t slot = size_;
  if (size_ < capacity_) {
    ++size_;
  } else {
    slot = 0;
    for (size_t i = 1; i < size_; ++i) {
      if (records_[i].lastSeenMs < records_[slot].lastSeenMs) slot = i;
    }
  }

  records_[slot] = RemoteIdRecord{};
  std::memcpy(records_[slot].mac, mac, 6);
  records_[slot].transportMask = transportBit(transport);
  records_[slot].rssi = rssi;
  records_[slot].firstSeenMs = nowMs;
  records_[slot].lastSeenMs = nowMs;
  records_[slot].packetCount = 1;
  records_[slot].rateWindowStartedMs = nowMs;
  records_[slot].rateWindowPackets = 1;
  return records_[slot];
}

RemoteIdRecord* RemoteIdStore::findByUasId(const char* uasId) {
  if (uasId == nullptr || uasId[0] == '\0') return nullptr;
  for (size_t i = 0; i < size_; ++i) {
    if (records_[i].hasUasId && std::strncmp(records_[i].uasId, uasId, 21) == 0)
      return &records_[i];
  }
  return nullptr;
}

const RemoteIdRecord* RemoteIdStore::findByUasId(const char* uasId) const {
  return const_cast<RemoteIdStore*>(this)->findByUasId(uasId);
}

RemoteIdRecord* RemoteIdStore::findByMac(const uint8_t mac[6]) {
  if (mac == nullptr) return nullptr;
  for (size_t i = 0; i < size_; ++i) {
    if (sameMac(records_[i].mac, mac)) return &records_[i];
  }
  return nullptr;
}

const RemoteIdRecord* RemoteIdStore::findByMac(const uint8_t mac[6]) const {
  return const_cast<RemoteIdStore*>(this)->findByMac(mac);
}

RemoteIdRecord* RemoteIdStore::at(size_t index) {
  return index < size_ ? &records_[index] : nullptr;
}

const RemoteIdRecord* RemoteIdStore::at(size_t index) const {
  return index < size_ ? &records_[index] : nullptr;
}

size_t RemoteIdStore::size() const { return size_; }
size_t RemoteIdStore::capacity() const { return capacity_; }

size_t RemoteIdStore::pruneStale(uint32_t nowMs, uint32_t staleAfterMs,
                                 const char* preservedUasId,
                                 const uint8_t* preservedMac) {
  size_t removed = 0;
  size_t index = 0;
  while (index < size_) {
    const bool preserveUas = preservedUasId != nullptr &&
        records_[index].hasUasId &&
        std::strncmp(records_[index].uasId, preservedUasId, 21) == 0;
    const bool preserveMac = preservedMac != nullptr &&
        sameMac(records_[index].mac, preservedMac);
    if (!preserveUas && !preserveMac &&
        remoteIdIsStale(nowMs, records_[index].lastSeenMs, staleAfterMs)) {
      for (size_t move = index + 1; move < size_; ++move)
        records_[move - 1] = records_[move];
      records_[size_ - 1] = RemoteIdRecord{};
      --size_;
      ++removed;
      continue;
    }
    ++index;
  }
  return removed;
}

size_t RemoteIdStore::updateLifecycle(uint32_t nowMs, uint32_t lostAfterMs) {
  size_t newlyLost = 0;
  for (size_t i = 0; i < size_; ++i) {
    const bool lost = remoteIdIsStale(nowMs, records_[i].lastSeenMs, lostAfterMs);
    if (lost && !records_[i].isLost) {
      records_[i].isLost = true;
      ++records_[i].lostCount;
      records_[i].lastLostMs = nowMs;
      ++newlyLost;
    }
  }
  return newlyLost;
}

bool RemoteIdStore::eraseByMac(const uint8_t mac[6],
                               const RemoteIdRecord* except) {
  if (mac == nullptr) return false;
  for (size_t i = 0; i < size_; ++i) {
    if (&records_[i] == except || !sameMac(records_[i].mac, mac)) continue;
    for (size_t move = i + 1; move < size_; ++move)
      records_[move - 1] = records_[move];
    records_[size_ - 1] = RemoteIdRecord{};
    --size_;
    return true;
  }
  return false;
}

void RemoteIdStore::clear() {
  for (size_t i = 0; i < size_; ++i) records_[i] = RemoteIdRecord{};
  size_ = 0;
}

RemoteIdLayout remoteIdLayoutForDisplay(uint16_t width, uint16_t height) {
  const uint16_t shortEdge = width < height ? width : height;
  const uint16_t longEdge = width > height ? width : height;
  if (shortEdge >= 240 && longEdge >= 320) return RemoteIdLayout::SplitListGrid;
  if (shortEdge < 160 || longEdge < 200) return RemoteIdLayout::Compact;
  return RemoteIdLayout::ListOnly;
}

float remoteIdDistanceMeters(int32_t latAE7, int32_t lonAE7,
                             int32_t latBE7, int32_t lonBE7) {
  const double latA = latAE7 * 1e-7 * kDegToRad;
  const double latB = latBE7 * 1e-7 * kDegToRad;
  const double dLat = latB - latA;
  const double dLon = (lonBE7 - lonAE7) * 1e-7 * kDegToRad;
  const double sinLat = std::sin(dLat * 0.5);
  const double sinLon = std::sin(dLon * 0.5);
  const double a = sinLat * sinLat + std::cos(latA) * std::cos(latB) * sinLon * sinLon;
  return static_cast<float>(2.0 * kEarthRadiusM * std::atan2(std::sqrt(a), std::sqrt(1.0 - a)));
}

float remoteIdBearingDegrees(int32_t latAE7, int32_t lonAE7,
                             int32_t latBE7, int32_t lonBE7) {
  const double latA = latAE7 * 1e-7 * kDegToRad;
  const double latB = latBE7 * 1e-7 * kDegToRad;
  const double dLon = (lonBE7 - lonAE7) * 1e-7 * kDegToRad;
  const double y = std::sin(dLon) * std::cos(latB);
  const double x = std::cos(latA) * std::sin(latB) -
                   std::sin(latA) * std::cos(latB) * std::cos(dLon);
  double bearing = std::atan2(y, x) / kDegToRad;
  if (bearing < 0.0) bearing += 360.0;
  return static_cast<float>(bearing);
}

bool remoteIdIsStale(uint32_t nowMs, uint32_t lastSeenMs,
                     uint32_t staleAfterMs) {
  return static_cast<uint32_t>(nowMs - lastSeenMs) >= staleAfterMs;
}

bool remoteIdCoordinatesValid(int32_t latitudeE7, int32_t longitudeE7) {
  if (latitudeE7 < -900000000 || latitudeE7 > 900000000 ||
      longitudeE7 < -1800000000 || longitudeE7 > 1800000000)
    return false;
  return latitudeE7 != 0 || longitudeE7 != 0;
}

bool remoteIdIdentityMatches(const char* knownUasId, const uint8_t knownMac[6],
                             const RemoteIdRecord& record) {
  if (record.hasUasId && knownUasId != nullptr && knownUasId[0] != '\0')
    return std::strncmp(knownUasId, record.uasId, sizeof(record.uasId)) == 0;
  return knownMac != nullptr &&
         std::memcmp(knownMac, record.mac, sizeof(record.mac)) == 0;
}

bool remoteIdUpdateLocation(RemoteIdRecord& record, int32_t latitudeE7,
                            int32_t longitudeE7) {
  if (!remoteIdCoordinatesValid(latitudeE7, longitudeE7)) return false;
  if (!record.hasLocation) {
    record.latitudeE7 = latitudeE7;
    record.longitudeE7 = longitudeE7;
    record.hasLocation = true;
    ++record.locationRevision;
    return true;
  }
  if (record.latitudeE7 == latitudeE7 && record.longitudeE7 == longitudeE7)
    return false;

  double oldEastM = 0.0;
  double oldNorthM = 0.0;
  relativeMeters(record.latitudeE7, record.longitudeE7, latitudeE7, longitudeE7,
                 oldEastM, oldNorthM);
  for (size_t i = 0; i < record.historyCount; ++i) {
    record.history[i].eastM =
        clampMeters(static_cast<double>(record.history[i].eastM) + oldEastM);
    record.history[i].northM =
        clampMeters(static_cast<double>(record.history[i].northM) + oldNorthM);
  }
  RemoteIdHistoryPoint previous;
  previous.eastM = clampMeters(oldEastM);
  previous.northM = clampMeters(oldNorthM);
  if (previous.eastM != 0 || previous.northM != 0) {
    if (record.historyCount == REMOTE_ID_HISTORY_CAPACITY) {
      for (size_t i = 1; i < REMOTE_ID_HISTORY_CAPACITY; ++i)
        record.history[i - 1] = record.history[i];
      --record.historyCount;
    }
    record.history[record.historyCount++] = previous;
  }
  record.latitudeE7 = latitudeE7;
  record.longitudeE7 = longitudeE7;
  ++record.locationRevision;
  return true;
}

bool remoteIdShouldReplaceBasicId(uint8_t currentType, bool hasCurrent,
                                  uint8_t candidateType) {
  if (!hasCurrent || candidateType == currentType) return true;
  // ASTM permits multiple Basic IDs. Prefer the module's serial/device ID over
  // a CAA registration so an alternating transmitter has one stable identity.
  const uint8_t currentPriority = currentType == 1 ? 3 : currentType == 2 ? 2 : 1;
  const uint8_t candidatePriority =
      candidateType == 1 ? 3 : candidateType == 2 ? 2 : 1;
  return candidatePriority > currentPriority;
}

void remoteIdFormatDistanceKm(float distanceM, char* output,
                              size_t outputSize) {
  if (output == nullptr || outputSize == 0) return;
  if (!std::isfinite(distanceM) || distanceM < 0.0f) {
    output[0] = '\0';
    return;
  }
  std::snprintf(output, outputSize, "%.2fkm", distanceM / 1000.0f);
}

void remoteIdFormatGridRadius(float radiusM, char* output,
                              size_t outputSize) {
  if (output == nullptr || outputSize == 0) return;
  if (!std::isfinite(radiusM) || radiusM < 0.0f) {
    output[0] = '\0';
    return;
  }
  if (radiusM >= 1000.0f)
    std::snprintf(output, outputSize, "%.1fkm", radiusM / 1000.0f);
  else
    std::snprintf(output, outputSize, "%.0fm", radiusM);
}

void remoteIdFormatTransports(uint8_t transportMask, char* output,
                              size_t outputSize) {
  if (output == nullptr || outputSize == 0) return;
  output[0] = '\0';
  const struct {
    uint8_t bit;
    const char* label;
  } transports[] = {
      {static_cast<uint8_t>(RemoteIdTransport::WifiBeacon), "WB"},
      {static_cast<uint8_t>(RemoteIdTransport::WifiNan), "WN"},
      {static_cast<uint8_t>(RemoteIdTransport::BleLegacy), "B4"},
      {static_cast<uint8_t>(RemoteIdTransport::BleExtended), "B5"},
  };
  size_t used = 0;
  for (const auto& transport : transports) {
    if ((transportMask & transport.bit) == 0) continue;
    const int written = std::snprintf(output + used, outputSize - used,
                                      used == 0 ? "%s" : "+%s",
                                      transport.label);
    if (written < 0 || static_cast<size_t>(written) >= outputSize - used) {
      output[outputSize - 1] = '\0';
      return;
    }
    used += static_cast<size_t>(written);
  }
  if (used == 0) std::snprintf(output, outputSize, "--");
}

float remoteIdGridScaleMeters(const RemoteIdRecord* records, size_t count,
                              int32_t originLatE7, int32_t originLonE7,
                              float minimumRadiusM) {
  double farthest = minimumRadiusM;
  for (size_t i = 0; i < count; ++i) {
    const RemoteIdRecord& record = records[i];
    if (record.hasLocation)
      farthest = std::fmax(farthest, remoteIdDistanceMeters(
          originLatE7, originLonE7, record.latitudeE7, record.longitudeE7));
    if (record.hasLocation && record.historyCount != 0) {
      double currentEastM = 0.0;
      double currentNorthM = 0.0;
      relativeMeters(record.latitudeE7, record.longitudeE7,
                     originLatE7, originLonE7, currentEastM, currentNorthM);
      for (size_t j = 0; j < record.historyCount; ++j) {
        farthest = std::fmax(farthest, std::hypot(
            currentEastM + record.history[j].eastM,
            currentNorthM + record.history[j].northM));
      }
    }
    if (record.hasOperatorLocation)
      farthest = std::fmax(farthest, remoteIdDistanceMeters(
          originLatE7, originLonE7, record.operatorLatitudeE7,
          record.operatorLongitudeE7));
  }
  return static_cast<float>(farthest * 1.15);
}

RemoteIdGridPoint remoteIdProjectToGrid(int32_t pointLatE7, int32_t pointLonE7,
                                        int32_t originLatE7, int32_t originLonE7,
                                        int16_t width, int16_t height,
                                        float radiusM, int16_t padding) {
  double eastM = 0.0;
  double northM = 0.0;
  relativeMeters(pointLatE7, pointLonE7, originLatE7, originLonE7, eastM, northM);
  return projectMetersToGrid(eastM, northM, width, height, radiusM, padding);
}

RemoteIdGridPoint remoteIdProjectHistoryToGrid(
    const RemoteIdRecord& record, const RemoteIdHistoryPoint& historyPoint,
    int32_t originLatE7, int32_t originLonE7, int16_t width, int16_t height,
    float radiusM, int16_t padding) {
  if (!record.hasLocation) return RemoteIdGridPoint{};
  double eastM = 0.0;
  double northM = 0.0;
  relativeMeters(record.latitudeE7, record.longitudeE7,
                 originLatE7, originLonE7, eastM, northM);
  return projectMetersToGrid(eastM + historyPoint.eastM,
                             northM + historyPoint.northM,
                             width, height, radiusM, padding);
}
