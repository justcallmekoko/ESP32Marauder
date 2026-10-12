#pragma once

#include <stdint.h>
#include <stddef.h>

class CsiMotionModel {
 public:
  static constexpr size_t kMaxCarriers = 128;
  static constexpr size_t kHistorySize = 160;
  static constexpr uint8_t kPossibleMotionThreshold = 12;
  static constexpr uint8_t kMotionThreshold = 28;

  enum class State : uint8_t {
    CALIBRATING,
    INSUFFICIENT_SAMPLES,
    STILL,
    POSSIBLE_MOTION,
    MOTION,
  };

  void reset(uint32_t now_ms, uint32_t calibration_ms = 30000);
  void addSample(const int8_t* iq, size_t length, int8_t rssi, uint32_t now_ms);
  void tick(uint32_t now_ms, uint32_t accepted, uint32_t dropped);

  bool calibrated() const { return calibrated_; }
  uint8_t calibrationPercent(uint32_t now_ms) const;
  uint8_t motionScore() const { return motion_score_; }
  uint8_t amplitudeScore() const { return amplitude_score_; }
  uint8_t phaseScore() const { return phase_score_; }
  uint8_t correlationScore() const { return correlation_score_; }
  uint8_t reliability() const { return reliability_; }
  uint16_t samplesPerSecond() const { return samples_per_second_; }
  int8_t rssi() const { return last_rssi_; }
  State state() const { return state_; }
  const uint8_t* history() const { return history_; }
  size_t historyHead() const { return history_head_; }
  uint32_t calibrationDurationMs() const { return calibration_ms_; }

 private:
  float baseline_[kMaxCarriers] = {};
  float noise_[kMaxCarriers] = {};
  float previous_phase_[kMaxCarriers] = {};
  uint8_t history_[kHistorySize] = {};
  size_t carrier_count_ = 0;
  size_t history_head_ = 0;
  uint32_t started_ms_ = 0;
  uint32_t calibration_ms_ = 30000;
  uint32_t last_tick_ms_ = 0;
  uint32_t last_history_ms_ = 0;
  uint32_t previous_accepted_ = 0;
  uint32_t previous_dropped_ = 0;
  float rssi_mean_ = -100.0f;
  float rssi_deviation_ = 0.0f;
  float smoothed_score_ = 0.0f;
  float phase_baseline_ = 0.0f;
  float phase_noise_ = 0.01f;
  float correlation_baseline_ = 0.0f;
  float correlation_noise_ = 0.01f;
  uint16_t samples_per_second_ = 0;
  int8_t last_rssi_ = -127;
  uint8_t motion_score_ = 0;
  uint8_t amplitude_score_ = 0;
  uint8_t phase_score_ = 0;
  uint8_t correlation_score_ = 0;
  uint8_t reliability_ = 0;
  uint8_t motion_votes_ = 0;
  uint8_t still_votes_ = 0;
  bool calibrated_ = false;
  bool phase_initialized_ = false;
  State state_ = State::CALIBRATING;
};
