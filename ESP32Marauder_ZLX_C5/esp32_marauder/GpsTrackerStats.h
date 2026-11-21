#pragma once

#include <stdint.h>

namespace marauder {

class GpsTrackerStats {
 public:
  void reset(uint32_t now_ms);
  bool update(int32_t latitude_e6, int32_t longitude_e6, float accuracy_m,
              uint32_t now_ms);

  uint32_t elapsedMs(uint32_t now_ms) const;
  float distanceMeters() const { return distance_m_; }
  float speedMetersPerSecond() const { return speed_mps_; }
  uint32_t loggedPoints() const { return logged_points_; }
  bool hasFix() const { return has_fix_; }

  static float distanceBetweenMeters(int32_t latitude_a_e6,
                                     int32_t longitude_a_e6,
                                     int32_t latitude_b_e6,
                                     int32_t longitude_b_e6);

 private:
  uint32_t start_ms_ = 0;
  uint32_t last_fix_ms_ = 0;
  int32_t last_latitude_e6_ = 0;
  int32_t last_longitude_e6_ = 0;
  float distance_m_ = 0.0f;
  float speed_mps_ = 0.0f;
  uint32_t logged_points_ = 0;
  bool has_fix_ = false;
};

}  // namespace marauder
