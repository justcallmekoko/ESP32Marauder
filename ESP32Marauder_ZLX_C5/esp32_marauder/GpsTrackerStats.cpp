#include "GpsTrackerStats.h"

#include <math.h>

namespace marauder {
namespace {
constexpr double kDegreesToRadians = 0.017453292519943295;
constexpr double kEarthRadiusMeters = 6371000.0;
constexpr float kMaximumPlausibleSpeedMps = 120.0f;
}

void GpsTrackerStats::reset(uint32_t now_ms) {
  start_ms_ = now_ms;
  last_fix_ms_ = now_ms;
  last_latitude_e6_ = 0;
  last_longitude_e6_ = 0;
  distance_m_ = 0.0f;
  speed_mps_ = 0.0f;
  logged_points_ = 0;
  has_fix_ = false;
}

uint32_t GpsTrackerStats::elapsedMs(uint32_t now_ms) const {
  return now_ms - start_ms_;
}

float GpsTrackerStats::distanceBetweenMeters(int32_t latitude_a_e6,
                                             int32_t longitude_a_e6,
                                             int32_t latitude_b_e6,
                                             int32_t longitude_b_e6) {
  const double lat_a = latitude_a_e6 * 0.000001 * kDegreesToRadians;
  const double lat_b = latitude_b_e6 * 0.000001 * kDegreesToRadians;
  const double delta_lat = lat_b - lat_a;
  const double delta_lon =
      (longitude_b_e6 - longitude_a_e6) * 0.000001 * kDegreesToRadians;
  const double sin_lat = sin(delta_lat * 0.5);
  const double sin_lon = sin(delta_lon * 0.5);
  const double haversine = sin_lat * sin_lat +
      cos(lat_a) * cos(lat_b) * sin_lon * sin_lon;
  const double bounded_haversine = haversine > 1.0 ? 1.0 : haversine;
  const double arc = 2.0 * atan2(sqrt(bounded_haversine),
                                 sqrt(1.0 - bounded_haversine));
  return static_cast<float>(kEarthRadiusMeters * arc);
}

bool GpsTrackerStats::update(int32_t latitude_e6, int32_t longitude_e6,
                             float accuracy_m, uint32_t now_ms) {
  ++logged_points_;
  if (!has_fix_) {
    last_latitude_e6_ = latitude_e6;
    last_longitude_e6_ = longitude_e6;
    last_fix_ms_ = now_ms;
    has_fix_ = true;
    return true;
  }

  const uint32_t delta_ms = now_ms - last_fix_ms_;
  const float step_m = distanceBetweenMeters(last_latitude_e6_,
                                             last_longitude_e6_, latitude_e6,
                                             longitude_e6);
  const float jitter_floor_m = accuracy_m > 3.0f ? accuracy_m : 3.0f;
  const float maximum_step_m = delta_ms * 0.001f * kMaximumPlausibleSpeedMps;

  last_latitude_e6_ = latitude_e6;
  last_longitude_e6_ = longitude_e6;
  last_fix_ms_ = now_ms;

  if (delta_ms == 0 || step_m < jitter_floor_m || step_m > maximum_step_m) {
    speed_mps_ = 0.0f;
    return false;
  }

  distance_m_ += step_m;
  speed_mps_ = step_m / (delta_ms * 0.001f);
  return true;
}

}  // namespace marauder
