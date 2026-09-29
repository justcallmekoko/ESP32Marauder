#include "RemoteIdModel.h"

#include <cmath>
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
}  // namespace

RemoteIdStore::RemoteIdStore(RemoteIdRecord* records, size_t capacity)
    : records_(records), capacity_(capacity) {}

RemoteIdRecord& RemoteIdStore::observe(const uint8_t mac[6],
                                        RemoteIdTransport transport,
                                        int8_t rssi, uint32_t nowMs) {
  for (size_t i = 0; i < size_; ++i) {
    if (sameMac(records_[i].mac, mac)) {
      records_[i].transportMask |= transportBit(transport);
      records_[i].rssi = rssi;
      records_[i].lastSeenMs = nowMs;
      ++records_[i].packetCount;
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

float remoteIdGridScaleMeters(const RemoteIdRecord* records, size_t count,
                              int32_t originLatE7, int32_t originLonE7,
                              float minimumRadiusM) {
  double farthest = minimumRadiusM;
  for (size_t i = 0; i < count; ++i) {
    const RemoteIdRecord& record = records[i];
    if (record.hasLocation)
      farthest = std::fmax(farthest, remoteIdDistanceMeters(
          originLatE7, originLonE7, record.latitudeE7, record.longitudeE7));
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
  RemoteIdGridPoint point;
  if (width <= padding * 2 || height <= padding * 2 || radiusM <= 0.0f) return point;
  double eastM = 0.0;
  double northM = 0.0;
  relativeMeters(pointLatE7, pointLonE7, originLatE7, originLonE7, eastM, northM);
  const double halfW = (width - padding * 2) * 0.5;
  const double halfH = (height - padding * 2) * 0.5;
  point.x = static_cast<int16_t>(std::lround(width * 0.5 + eastM / radiusM * halfW));
  point.y = static_cast<int16_t>(std::lround(height * 0.5 - northM / radiusM * halfH));
  point.visible = point.x >= padding && point.x < width - padding &&
                  point.y >= padding && point.y < height - padding;
  return point;
}
